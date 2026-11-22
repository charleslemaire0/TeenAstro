// Copyright (c) 2026 Steven Rosenthal smr@dt3.org
// See LICENSE file in root directory for license terms.

//! CedarDetect provides efficient and accurate detection of stars in sky images.
//! Given an image, CedarDetect returns a list of detected star centroids expressed
//! in image pixel coordinates. The input image should be the camera's native full
//! resolution; centroid coordinates are always in the pixel space of whichever
//! image is supplied.
//!
//! Features:
//!
//! * Employs localized thresholding to tolerate changes in background levels
//!   across the image.
//! * Adapts to different image exposure levels.
//! * Estimates noise in the image and adapts the star detection threshold
//!   accordingly.
//! * Optionally classifies and rejects isolated hot pixels.
//! * Rejects trailed objects such as aircraft lights or satellites.
//! * Tolerates the presence of bright interlopers such as the moon or
//!   streetlights.
//! * Simple function call interface with few parameters aside from the input
//!   image.
//! * Fast! On a Raspberry Pi 4B, the execution time per 1M image pixels is
//!   usually less than 10ms, even when several dozen stars are present in the
//!   image.
//!
//! # Intended applications
//!
//! CedarDetect is designed to be used with astrometry and plate solving systems
//! such as [Cedar-Solve](https://github.com/smroid/cedar-solve). It can also be
//! incorporated into satellite star trackers.
//!
//! A goal of CedarDetect is to allow such applications to achieve fast response
//! times. CedarDetect contributes to this by running quickly and by tolerating a
//! degree of image noise allowing for shorter imaging integration times.
//!
//! # Star detection fidelity
//!
//! Like any detection algorithm, CedarDetect produces both false negatives (failures
//! to detect actual stars) and false positives (spurious star detections). The
//! Caveats section below mentions some causes of false negatives.
//!
//! False positives can occur as you reduce the `sigma` parameter when calling
//! [get_stars_from_image()] in an attempt to increase the number of detected
//! stars. Although CedarDetect requires evidence from multiple pixels to qualify
//! each star detection (reducing the occurrence of false positives), noise
//! inevitably wins if you push `sigma` too low.
//!
//! # Algorithm
//!
//! CedarDetect spends most of its time on a single efficient pass over the input
//! image. This generates a set of star candidates that are subjected to further
//! scrutiny. The candidates number in the hundreds or perhaps thousands (thus
//! much less than the number of image pixels), so the candidate evaluation
//! usually takes only a fraction of the overall time.
//!
//! The code comments have much more detail.
//!
//! # Caveats
//!
//! ## Star extraction only
//!
//! CedarDetect is designed to identify only star-like spots. It is not a
//! generalized astronomical source extraction system.
//!
//! ## Crowding
//!
//! The criteria used by CedarDetect to efficiently detect stars are designed
//! around the characteristics of a star image's pixels compared to surrounding
//! pixels. CedarDetect can thus be confused when stars are too closely spaced,
//! or a star is close to a hot pixel, or if image shake causes star images to
//! be "doubled". Such situations will usually cause closely spaced stars to
//! fail to be detected. Note that for applications such as plate solving, this
//! is probably for the better.
//!
//! ## Imaging requirements
//!
//! * CedarDetect supports only 8-bit grayscale images; color images or images with
//!   greater bit depth must be converted before calling
//!   [get_stars_from_image()].
//! * The imaging exposure time and sensor gain are usually chosen by the caller
//!   to yield a desired number of faint star detections. In so doing, if a
//!   bright star is overexposed to a degree that it bleeds into too many
//!   adjacent pixels, CedarDetect will reject the bright star.
//! * Pixel scale and focusing are crucial:
//!   * If star images are too extended w.r.t. the pixel grid, CedarDetect might not
//!     detect the stars. You'll either need to tighten the focus or use the pixel
//!     binning option when calling [get_stars_from_image()].
//!   * If stars are too small w.r.t. the pixel grid, CedarDetect might
//!     mis-classify stars as isolated hot pixels. A simple remedy is to
//!     slightly defocus causing stars to occupy a central peak pixel with a bit
//!     of spread into immediately adjacent pixels.
//! * CedarDetect does not tolerate more than maybe a single pixel of motion blur.
//!
//! ## Centroid estimation
//!
//! CedarDetect computes a sub-pixel centroid position for each detected star
//! as the flux-weighted center of gravity of the star's pixels. This suffices
//! for many purposes, but a more exacting application might want to do its own
//! centroiding:
//!
//! * If a higher bit-depth image was converted to 8 bit for calling
//!   [get_stars_from_image()], centroiding in the original image may yield
//!   better accuracy.
//! * The application can use a more artful centroiding technique such as
//!   gaussian fitting.
//!
//! ## Lens distortions
//!
//! Optical imaging systems can produce distortions that affect CedarDetect in two
//! ways:
//!
//! * Optical aberrations can cause star images to deviate from ideal small
//!   condensed spots. CedarDetect will fail to detect stars that are too distorted,
//!   such as near the corners of a wide field image.
//! * Lens pincushion and/or barrel distortions can cause reported star positions
//!   to deviate from their true positions. CedarDetect reports star centroids in
//!   the image's x/y coordinates; it is up to the application to apply any needed
//!   lens distortion corrections when doing astrometry (e.g. plate solving).

use std::cmp;
use std::sync::Arc;
use std::time::Instant;

use image::GrayImage;
use imageproc::rect::Rect;
use log::debug;

use crate::histogram_funcs::{HistogramStats,
                             remove_stars_from_histogram,
                             stats_for_histogram};
use crate::image_funcs::{bin_2x2, get_row_scanner};

// Compare two floats, treating NaN as equal to other values. This allows sorting
// and comparison operations to complete without panicking if NaN values are
// encountered, which could happen due to edge cases in calculations.
fn compare_floats(a: &f64, b: &f64) -> std::cmp::Ordering {
    a.partial_cmp(b).unwrap_or(std::cmp::Ordering::Equal)
}

#[inline(always)]
fn for_each_pixel_in_roi<F>(image: &GrayImage, roi: &Rect, mut f: F)
where
    F: FnMut(i32, i32, u8),
{
    let (width, height) = {
        let (w, h) = image.dimensions();
        (w as usize, h as usize)
    };
    assert!(roi.left() >= 0);
    assert!(roi.top() >= 0);
    assert!(roi.right() < width as i32);
    assert!(roi.bottom() < height as i32);
    let raw = image.as_raw();
    let left = roi.left() as usize;
    let top = roi.top() as usize;
    let bottom = roi.bottom() as usize;
    let w = roi.width() as usize;
    if w == 0 || roi.height() == 0 {
        return;
    }
    for y in top..=bottom {
        let row_start = y * width;
        let row_slice = &raw[row_start + left .. row_start + left + w];
        for (i, &pixel) in row_slice.iter().enumerate() {
            f((left + i) as i32, y as i32, pixel);
        }
    }
}

#[inline(always)]
fn for_each_perimeter_pixel<F>(image: &GrayImage, roi: &Rect, mut f: F)
where
    F: FnMut(i32, i32, u8),
{
    let (width, height) = {
        let (w, h) = image.dimensions();
        (w as usize, h as usize)
    };
    assert!(roi.left() >= 0);
    assert!(roi.top() >= 0);
    assert!(roi.right() < width as i32);
    assert!(roi.bottom() < height as i32);
    let raw = image.as_raw();
    let left = roi.left() as usize;
    let top = roi.top() as usize;
    let bottom = roi.bottom() as usize;
    let w = roi.width() as usize;
    if w == 0 || roi.height() == 0 {
        return;
    }
    let right = left + w - 1;

    // Top row.
    let top_row_start = top * width;
    let top_row_slice = &raw[top_row_start + left .. top_row_start + left + w];
    for (i, &pixel) in top_row_slice.iter().enumerate() {
        f((left + i) as i32, top as i32, pixel);
    }
    if bottom > top {
        // Middle rows.
        for y in (top + 1)..bottom {
            let row_start = y * width;
            f(left as i32, y as i32, raw[row_start + left]);
            if w > 1 {
                f(right as i32, y as i32, raw[row_start + right]);
            }
        }
        // Bottom row.
        let bot_row_start = bottom * width;
        let bot_row_slice = &raw[bot_row_start + left .. bot_row_start + left + w];
        for (i, &pixel) in bot_row_slice.iter().enumerate() {
            f((left + i) as i32, bottom as i32, pixel);
        }
    }
}

// Given a candidate pixel, examines that pixel within a 7 pixel horizontal
// context (the candidate pixel plus three neighbors on each side).
//
// We label the pixels as: |lb lm l C r rm rb| where:
// lb: left border
// lm: left margin
// l:  left neighbor
// C:  center or candidate pixel
// r:  right neighbor
// rm: right margin
// rb: right border
//
// When the candidate pixel is the center pixel of a horizontal cut through a
// star, we will observe/require the following:
//
// * The center pixel will be the brightest.
// * The outermost border pixels are taken to reflect the local sky background
//   value.
// * The center pixel will be brighter than the sky background by a
//   statistically significant amount.
//
// If a 7 pixel horizontal window satisfies these (and other; see the code)
// criteria, the center pixel is flagged as being a candidate for futher
// analysis. If any of these criteria are not met, the center pixel is deemed to
// be NOT a star.
//
// Note that the left and right "margin" pixels don't figure into the above
// narration; a 5-pixel horizontal window might have been used instead. The
// margin pixels allow a slightly higher degree of star defocus compared to what
// a 5 pixel window would permit.
//
// Statistical significance is defined as a 'sigma' multiple of the measured
// image noise level. The `sigma_noise_2` argument is 2x the sigma*noise value,
// and the `sigma_noise_3` argument is 3x the sigma*noise value.
//
// Returns: Whether the gate's center pixel is a star candidate or is
//   uninteresting.
#[derive(Debug, Eq, PartialEq)]
enum ResultType {
    Uninteresting,
    Candidate,
}

fn gate_star_1d(gate: &[u8], sigma_noise_2: i16, sigma_noise_3: i16)
                -> ResultType {
    debug_assert!(sigma_noise_2 > 0);
    debug_assert!(sigma_noise_3 > 0);
    debug_assert!(sigma_noise_2 <= sigma_noise_3);
    // Examining assembler output suggests that fetching these exterior pixels
    // first eliminates bounds checks on the interior pixels. I would have
    // thought that the compiler would do this... in any case, the measured
    // performance doesn't seem to change.
    let lb = gate[0] as i16;
    let rb = gate[6] as i16;

    let lm = gate[1] as i16;
    let l = gate[2] as i16;
    let c = gate[3] as i16;
    let r = gate[4] as i16;
    let rm = gate[5] as i16;

    // Center pixel must be sigma * estimated noise brighter than the estimated
    // background. Do this test first, because it eliminates the vast majority
    // of candidates.
    let est_background_2 = lb + rb;
    let center_minus_background_2 = c + c - est_background_2;
    if center_minus_background_2 < sigma_noise_2 {
        return ResultType::Uninteresting;
    }
    // Center pixel must be at least as bright as its immediate left/right
    // neighbors.
    if l > c || c < r {
        return ResultType::Uninteresting;
    }
    // Center pixel must be strictly brighter than its left/right margin.
    if lm >= c || c <= rm {
        return ResultType::Uninteresting;
    }
    if l == c {
        // Break tie between left and center.
        if lm > r {
            // Left will have been the center of its own candidate entry.
            return ResultType::Uninteresting;
        }
    }
    if c == r {
        // Break tie between center and right.
        if l <= rm {
            // Right will be the center of its own candidate entry.
            return ResultType::Uninteresting;
        }
    }
    // We require the border pixels to be ~uniformly dark. See if there is too
    // much brightness difference between the border pixels.
    // The 3x sigma_noise threshold is empirically chosen to yield a low
    // rejection rate for actual sky background border pixels.
    let border_diff = (lb - rb).abs();
    if border_diff > sigma_noise_3 {
        return ResultType::Uninteresting;
    }
    // We have a candidate star from our 1d analysis!
    debug!("candidate: {:?}", gate);
    ResultType::Candidate
}

/// Public wrapper around gate_star_1d() for use by external NEON row scanners.
/// `gate` must be a 7-element slice centered on the candidate pixel.
/// Returns true if the pixel qualifies as a star candidate.
pub fn gate_star_1d_pub(gate: &[u8], sigma_noise_2: i16, sigma_noise_3: i16) -> bool {
    gate_star_1d(gate, sigma_noise_2, sigma_noise_3) == ResultType::Candidate
}

#[derive(Copy, Clone, Debug)]
struct CandidateFrom1D {
    x: i32,
    y: i32,
}

// Default per-row implementation: samples every 64 bytes to estimate the row
// minimum, derives a threshold from it, then calls gate_star_1d() for each
// pixel above the threshold.
fn scan_row_for_candidates_default(
    image: &GrayImage,
    rownum: usize,
    sigma_noise_2: i16,
    sigma_noise_3: i16,
    candidates: &mut Vec<(i32, i32)>,
) {
    let width = image.width() as usize;
    let row_pixels = &image.as_raw()[rownum * width..(rownum + 1) * width];

    // First pass: sample every cache line (64 bytes) to estimate row minimum.
    let mut row_min = 255u8;
    for i in (0..row_pixels.len()).step_by(64) {
        row_min = row_min.min(row_pixels[i]);
    }
    let threshold = row_min.saturating_add(sigma_noise_2 as u8 / 2);

    // Second pass: pixel loop, calling gate_star_1d() for pixels above threshold.
    for center_x in 3..(row_pixels.len() - 3) {
        let center_pixel = row_pixels[center_x];
        if center_pixel >= threshold {
            let gate = &row_pixels[center_x - 3..center_x + 4];
            if gate_star_1d(gate, sigma_noise_2, sigma_noise_3) == ResultType::Candidate {
                candidates.push((center_x as i32, rownum as i32));
            }
        }
    }
}

// Applies scan_row_for_candidates at all rows of the image to identify star
// candidates for further screening.
//
// The outermost three rows are skipped. gate_star_2d() rejects any blob whose
// core comes within three pixels of an image edge, so candidates found there
// could never become stars; skipping them also lets classify_pixel() rely on
// the rows above and below a candidate existing.
fn scan_image_for_candidates(image: &GrayImage,
                             noise_estimate: f64,
                             sigma: f64)
                             -> Vec<CandidateFrom1D>
{
    let row_scan_start = Instant::now();
    let width = image.width() as usize;
    let height = image.height() as usize;

    let sigma_noise_2 = cmp::max((2.0 * sigma * noise_estimate + 0.5) as i16, 2);
    let sigma_noise_3 = cmp::max((3.0 * sigma * noise_estimate + 0.5) as i16, 3);

    let scan_row = get_row_scanner().unwrap_or(scan_row_for_candidates_default);

    let estimated_candidates = (width * height) / 10000;
    let mut xy_candidates: Vec<(i32, i32)> = Vec::with_capacity(estimated_candidates);
    for rownum in 3..height.saturating_sub(3) {
        scan_row(image, rownum, sigma_noise_2, sigma_noise_3, &mut xy_candidates);
    }

    let candidates: Vec<CandidateFrom1D> = xy_candidates
        .into_iter()
        .map(|(x, y)| CandidateFrom1D { x, y })
        .collect();

    debug!("Image scan found {} candidates in {:?}",
           candidates.len(), row_scan_start.elapsed());
    candidates
}

#[derive(Debug)]
struct Blob {
    candidates: Vec<CandidateFrom1D>,

    // If candidates is empty, that means this blob has been merged into
    // another blob. None means no merge has occurred.
    recipient_blob: Option<usize>,
}

#[derive(Copy, Clone)]
struct LabeledCandidate {
    candidate: CandidateFrom1D,
    blob_id: usize,
}

// The scan_image_for_candidates() function can produce multiple candidates for
// the same star. This typically happens for a bright star that has some
// vertical extent such that multiple rows' horizontal cuts through the star
// all flag it as a candidate.
//
// The form_blobs_from_candidates() function gathers connected candidates into
// blobs which will be further analyzed to determine if they are stars.
fn form_blobs_from_candidates(candidates: Vec<CandidateFrom1D>, max_y: usize)
                              -> Vec<Blob> {
    let blobs_start = Instant::now();
    let mut labeled_candidates_by_row =
        vec![Vec::<LabeledCandidate>::new(); max_y + 1];

    let mut blobs: Vec<Blob> = Vec::with_capacity(candidates.len());
    // Create an initial singular blob for each candidate.
    for (next_blob_id, candidate) in candidates.into_iter().enumerate() {
        blobs.push(Blob{candidates: vec![candidate],
                        recipient_blob: None});
        labeled_candidates_by_row[candidate.y as usize].push(
            LabeledCandidate{candidate, blob_id: next_blob_id});
    }
    // Merge adjacent blobs. Within a row blobs are not adjacent (by the nature of
    // how row scanning identifies candidates), so we just need to look for vertical
    // adjacencies.
    // Start processing at row 1 so we can look to previous row for blob merges.
    for rownum in 1..labeled_candidates_by_row.len() {
        for rc in &labeled_candidates_by_row[rownum] {
            let rc_pos = rc.candidate.x;
            // See if rc is adjacent to any candidates in the previous row.
            // This is fast since rows usually have very few candidates.
            for prev_row_rc in &labeled_candidates_by_row[rownum - 1] {
                let prev_row_rc_pos = prev_row_rc.candidate.x;
                if prev_row_rc_pos < rc_pos - 3 {
                    continue;
                }
                if prev_row_rc_pos > rc_pos + 3 {
                    break;
                }
                // Adjacent to a candidate in the previous row. Absorb the previous
                // row blob's candidates.
                let recipient_blob_id = rc.blob_id;
                let mut donor_blob_id = prev_row_rc.blob_id;
                let mut donated_candidates: Vec<CandidateFrom1D>;
                loop {
                    let donor_blob = &mut blobs[donor_blob_id];
                    if !donor_blob.candidates.is_empty() {
                        donated_candidates = donor_blob.candidates.drain(..).collect();
                        assert_eq!(donor_blob.recipient_blob, None);
                        donor_blob.recipient_blob = Some(recipient_blob_id);
                        let recipient_blob = &mut blobs[recipient_blob_id];
                        recipient_blob.candidates.append(&mut donated_candidates);
                        break;
                    }
                    // donor_blob's candidates got merged to another blob.
                    let merged_blob_id = donor_blob.recipient_blob
                        .expect("if candidates are empty, must have been merged");
                    if merged_blob_id == recipient_blob_id {
                        break;  // Already merged it.
                    }
                    donor_blob_id = merged_blob_id;
                }
            }
        }
    }
    // Return non-empty blobs. Note that the blob merging we just did will leave
    // some empty entries in the `blobs` mapping.
    let mut non_empty_blobs = Vec::<Blob>::new();
    for blob in blobs {
        if !blob.candidates.is_empty() {
            assert_eq!(blob.recipient_blob, None);
            debug!("got blob {:?}", blob);
            non_empty_blobs.push(blob);
        }
    }
    debug!("Found {} blobs in {:?}", non_empty_blobs.len(), blobs_start.elapsed());
    non_empty_blobs
}

/// An isolated hot pixel detected by [get_stars_from_image()].
#[derive(Debug, Copy, Clone)]
pub struct HotPixel {
    /// Location in the pixel space of the `image` argument passed to
    /// [get_stars_from_image()].
    pub x: u32,
    pub y: u32,
    /// Interpolated neighbor average substituted for the hot pixel value.
    pub replacement_value: u8,
}

/// Summarizes a star-like spot found by [get_stars_from_image()].
#[derive(Debug, Copy, Clone)]
pub struct StarDescription {
    /// Location of star centroid in the pixel space of the `image` argument
    /// passed to [get_stars_from_image()]. (0.5, 0.5) corresponds to the center
    /// of that image's upper left pixel.
    pub centroid_x: f64,
    pub centroid_y: f64,

    /// Value of the brightest pixel in this star's region. Not background
    /// subtracted.
    pub peak_value: u8,

    /// Sum of the u8 pixel values of the star's region. The estimated
    /// background is subtracted.
    pub brightness: f64,

    /// Count of saturated pixel values.
    pub num_saturated: u16,
}

// The gate_star_2d() function is the 2-D version of gate_star_1d(). While the
// latter is simplistic because it is applied to every single image pixel, the
// gate_star_2d() function can be more thorough because it is only applied to
// hundreds or maybe thousands of candidates.
//
// A Blob is one or more candidates from gate_star_1d(), grouped together
// by form_blobs_from_candidates(). We define the following terms:
//
// Core: this is the bounding box that includes all of the input Blob's pixel
//   coordinates. In most cases the core consists of a single pixel, but for
//   brighter stars it can span multiple rows/columns. Confusing non-star
//   regions of the image (such as the lunar terminator) can also yield
//   multi-pixel cores.
// Neighbors: the single pixel box surrounding the core.
// Margin: the single pixel box surrounding neighbors.
// Perimeter: the single pixel box surrounding margin.
//
// When the core contains a star, we will observe/require the following:
//
// * Core is not too large.
// * Core is brightest.
// * The perimeter pixels are taken to reflect the local sky background value.
// * The core is brighter than the sky background by a statistically significant
//   amount.
//
// If a Blob and its surroundings satisfies these (and other; see the code)
// criteria, a StarDescription is generated by performing centroiding on the
// core+neighbor pixels. If any of these criteria are not met, the Blob
// is deemed to be NOT a star.
//
// Note that the "margin" box isn't mentioned in the above narration. The margin
// box allows some additional star defocus by pushing the sky background pixels
// (perimeter box) out a bit.
//
// Statistical significance is defined as a `sigma` multiple of the
// `noise_estimate`.
//
// image: This is either the original full resolution image, or a 2x2, 4x4, or
//     8x8 binned image.
//
// higher_res_image: if binning == 1, this is the same as 'image'. For other
//     binning values, this is the one-less binned image preceding 'image' in
//     the binning cascade. For example, if binning==4, image is 4x binned and
//     higher_res_image is only 2x binned.
//
// Regardless of whether `image` is the original or a 2x, 4x, or 8x binning,
//     we arrange to do centroiding on the higher resolution image.
//
// binning: 1 (no binning), 2, 4, or 8.
//
// noise_estimate: the noise level of `image`.
//
// higher_res_noise_estimate: the noise level of `higher_res_image`.

fn gate_star_2d(
    blob: &Blob,
    image: &GrayImage,
    higher_res_image: &GrayImage,
    binning: u32,
    noise_estimate: f64, higher_res_noise_estimate: f64, sigma: f64,
    max_width: u32, max_height: u32) -> Option<StarDescription>
{
    let (image_width, image_height) = image.dimensions();
    // Compute the bounding box of all of the blob's center coords.
    let mut x_min = u32::MAX;
    let mut x_max = 0_u32;
    let mut y_min = u32::MAX;
    let mut y_max = 0_u32;
    for candidate in &blob.candidates {
        x_min = cmp::min(x_min, candidate.x as u32);
        x_max = cmp::max(x_max, candidate.x as u32);
        y_min = cmp::min(y_min, candidate.y as u32);
        y_max = cmp::max(y_max, candidate.y as u32);
    }
    let core_x_min = x_min as i32;
    let core_x_max = x_max as i32;
    let core_y_min = y_min as i32;
    let core_y_max = y_max as i32;
    let core_width = (core_x_max - core_x_min) as u32 + 1;
    let core_height = (core_y_max - core_y_min) as u32 + 1;

    // Define bounding box for core.
    let core = Rect::at(core_x_min, core_y_min)
        .of_size(core_width, core_height);

    // Reject blob if it is too big.
    if core_width > max_width || core_height > max_height {
        debug!("Blob {:?} too large", core);
        return None;
    }
    // Reject blob if its expansion goes past an image boundary.
    if core_x_min - 3 < 0 || core_x_max + 3 >= image_width as i32 ||
        core_y_min - 3 < 0 || core_y_max + 3 >= image_height as i32
    {
        debug!("Blob {:?} too close to edge", core);
        return None;
    }

    // Expand core bounding box by three pixels in all directions.
    let neighbors = Rect::at(core_x_min - 1, core_y_min - 1)
        .of_size(core_width + 2, core_height + 2);
    let margin = Rect::at(core_x_min - 2, core_y_min - 2)
        .of_size(core_width + 4, core_height + 4);
    let perimeter = Rect::at(core_x_min - 3, core_y_min - 3)
        .of_size(core_width + 6, core_height + 6);

    // Compute average of pixels in core.
    let mut core_sum: i32 = 0;
    let mut core_count: i32 = 0;
    for_each_pixel_in_roi(image, &core, |_x, _y, pixel_value| {
        core_sum += i32::from(pixel_value);
        core_count += 1;
    });
    let core_mean = core_sum as f64 / core_count as f64;

    if core_width >= 3 && core_height >= 3 {
        // Require that the "inner" core be at least as bright as the outer
        // core. This covers the case where the blob's candidates are e.g. a
        // sunlit crater rim at the lunar terminator. These candidates could
        // form an arc shape with a dark interior. Usually such cases will be
        // rejected by the limit on the blob size, but we add an inner core
        // brightness criterion here for completeness.
        let mut outer_core_sum: i32 = 0;
        let mut outer_core_count: i32 = 0;
        for_each_perimeter_pixel(image, &core, |_x, _y, pixel_value| {
            outer_core_sum += i32::from(pixel_value);
            outer_core_count += 1;
        });
        let outer_core_mean = outer_core_sum as f64 / outer_core_count as f64;
        // When including the inner core (core_mean), we should be at least as
        // bright as when excluding the inner core (outer_core_mean).
        if core_mean < outer_core_mean {
            debug!("Overall core average {} is less than outer core average {} for blob {:?}",
                   core_mean, outer_core_mean, core);
            return None;
        }
    }

    // Compute average of pixels in box immediately surrounding core.
    let mut neighbor_sum: i32 = 0;
    let mut neighbor_count: i32 = 0;
    for_each_perimeter_pixel(image, &neighbors, |x, y, pixel_value| {
        let is_corner =
            (x == neighbors.left() || x == neighbors.right()) &&
            (y == neighbors.top() || y == neighbors.bottom());
        if !is_corner {
            neighbor_sum += i32::from(pixel_value);
            neighbor_count += 1;
        }
    });
    let neighbor_mean = neighbor_sum as f64 / neighbor_count as f64;
    // Core average must be at least as bright as the neighbor average.
    if core_mean < neighbor_mean {
        debug!("Core average {} is less than neighbor average {} for blob {:?}",
               core_mean, neighbor_mean, core);
        return None;
    }

    // Compute average of pixels in next box out; this is one pixel inward from
    // the outer perimeter.
    let mut margin_sum: i32 = 0;
    let mut margin_count: i32 = 0;
    for_each_perimeter_pixel(image, &margin, |_x, _y, pixel_value| {
        margin_sum += i32::from(pixel_value);
        margin_count += 1;
    });
    let margin_mean = margin_sum as f64 / margin_count as f64;
    if core_mean <= margin_mean {
        debug!("Core average {} is not greater than margin average {} for blob {:?}",
               core_mean, margin_mean, core);
        return None;
    }

    // Gather information from the outer perimeter. These pixels represent the
    // background.
    let mut perimeter_sum: i32 = 0;
    let mut perimeter_count: i32 = 0;
    let mut perimeter_min = image.get_pixel(perimeter.left() as u32,
                                            perimeter.top() as u32).0[0];
    let mut perimeter_max = perimeter_min;
    for_each_perimeter_pixel(image, &perimeter, |_x, _y, pixel_value| {
        perimeter_sum += i32::from(pixel_value);
        perimeter_count += 1;
        if pixel_value < perimeter_min {
            perimeter_min = pixel_value;
        } else if pixel_value > perimeter_max {
            perimeter_max = pixel_value;
        }
    });
    let background_est = perimeter_sum as f64 / perimeter_count as f64;
    debug!("background: {} for blob {:?}", background_est, core);

    // We require the perimeter pixels to be ~uniformly dark. See if any
    // perimeter pixel is too bright compared to the darkest perimeter
    // pixel.
    // The 1.5x sigma_noise threshold is empirically chosen to yield a low
    // rejection rate for actual sky background perimeter pixels.
    if (i32::from(perimeter_max) - i32::from(perimeter_min)) as f64 >
        1.5 * sigma * noise_estimate {
        debug!("Perimeter too varied for blob {:?}", core);
        return None;
    }

    // Verify that core average exceeds background by sigma * noise.
    if core_mean - background_est < sigma * noise_estimate {
        debug!("Core too weak for blob {:?}", core);
        return None;
    }

    // Star passes all of the 2d gates!

    let brightness;
    let num_saturated;
    let mut x;
    let mut y;
    let peak_value;
    if binning != 1 {
        // The `image` is binned. Compute moments using the higher-res image,
        // which is 2x less binned than `image`. Translate the margin from
        // detection-image space to higher_res_image space (scale=2), centroid
        // there, then scale up by binning/2 to reach captured-image space.
        let left = margin.left() as u32 * 2;
        let top = margin.top() as u32 * 2;
        let width = margin.width() * 2;
        let height = margin.height() * 2;
        let adj_width = cmp::min(left + width,
                                 higher_res_image.width()) - left;
        let adj_height = cmp::min(top + height,
                                  higher_res_image.height()) - top;
        let higher_res_margin =
            Rect::at(left as i32, top as i32).of_size(adj_width, adj_height);
        (brightness, num_saturated, peak_value) =
            compute_brightness(higher_res_image, &higher_res_margin);
        (x, y) = compute_centroid(higher_res_image, &higher_res_margin,
                                  higher_res_noise_estimate);
        let upsample = (binning / 2) as f64;
        // Apply the +0.5 pixel-center offset in higher-res space before
        // upsampling, so the offset scales correctly to captured-image space.
        x = (x + 0.5) * upsample;
        y = (y + 0.5) * upsample;
    } else {
        (brightness, num_saturated, peak_value) =
            compute_brightness(image, &margin);
        (x, y) = compute_centroid(image, &margin, noise_estimate);
        x += 0.5;
        y += 0.5;
    }
    Some(StarDescription{centroid_x: x,
                         centroid_y: y,
                         peak_value, brightness, num_saturated})
}

// Computes the background-subtracted brightness of the 2d image region.
// The outer perimeter of the bounding box is used for background
// estimation; the inner pixels of the bounding box are background
// subtracted and summed to form the brightness value.
// Returns: (summed pixel values, count of saturated pixels, peak pixel value)
fn compute_brightness(image: &GrayImage, bounding_box: &Rect) -> (f64, u16, u8) {
    // The inset below discards a one pixel ring, so anything smaller than 3x3
    // leaves nothing to sum (and a width below 2 would underflow).
    debug_assert!(bounding_box.width() >= 3 && bounding_box.height() >= 3,
                  "bounding_box {:?} too small to inset", bounding_box);

    let mut boundary_sum: i32 = 0;
    let mut boundary_count: i32 = 0;
    for_each_perimeter_pixel(image, bounding_box, |_x, _y, pixel_value| {
        boundary_sum += pixel_value as i32;
        boundary_count += 1;
    });
    let background_est = boundary_sum as f64 / boundary_count as f64;

    let inset = Rect::at(bounding_box.left() + 1, bounding_box.top() + 1)
        .of_size(bounding_box.width() - 2, bounding_box.height() - 2);

    let mut num_saturated = 0;
    let mut sum = 0.0;
    let mut peak_value: u8 = 0;
    // The background-subtracted pixel values are deliberately not clamped at
    // zero here, in contrast to the weights in compute_centroid(). Background
    // noise is symmetric about background_est, so letting the negative values
    // cancel the positive ones keeps the brightness unbiased. Only the total is
    // clamped, and only to avoid reporting a negative brightness.
    for_each_pixel_in_roi(image, &inset, |_x, _y, pixel_value| {
        if pixel_value == 255_u8 {
            num_saturated += 1;
        }
        if pixel_value > peak_value {
            peak_value = pixel_value;
        }
        sum += pixel_value as f64 - background_est;
    });
    (f64::max(sum, 0.0), num_saturated, peak_value)
}

// Computes the flux-weighted centroid of the star within `bounding_box`, at
// sub-pixel resolution.
//
// The perimeter of `bounding_box` is taken to represent the local sky
// background; that mean plus a multiple of `noise_estimate` defines a soft
// threshold. Each pixel then contributes weight max(value - threshold, 0),
// so background pixels contribute little or nothing while the star's profile
// contributes in proportion to its flux above the sky.
//
// The returned coordinates use the same "pixel index" convention as the
// input box: an integer value j denotes the center of pixel j. Callers add
// 0.5 to reach the continuous convention in which pixel j spans [j, j+1).
fn compute_centroid(image: &GrayImage, bounding_box: &Rect,
                    noise_estimate: f64) -> (f64, f64) {
    // Soft threshold above the background, in units of the noise. This
    // suppresses the noise pedestal that would otherwise pull the centroid
    // toward the center of `bounding_box`.
    const THRESHOLD_SIGMA: f64 = 1.0;

    let mut boundary_sum: f64 = 0.0;
    let mut boundary_count: f64 = 0.0;
    for_each_perimeter_pixel(image, bounding_box, |_x, _y, pixel_value| {
        boundary_sum += pixel_value as f64;
        boundary_count += 1.0;
    });
    let background_est = boundary_sum / boundary_count;
    let threshold = background_est + THRESHOLD_SIGMA * noise_estimate;

    let mut weight_sum = 0.0;
    let mut weighted_x = 0.0;
    let mut weighted_y = 0.0;
    for_each_pixel_in_roi(image, bounding_box, |x, y, pixel_value| {
        let weight = f64::max(pixel_value as f64 - threshold, 0.0);
        weight_sum += weight;
        weighted_x += weight * x as f64;
        weighted_y += weight * y as f64;
    });
    if weight_sum <= 0.0 {
        // Nothing rose above the threshold, so we have no information beyond
        // the box itself; report its center.
        return (bounding_box.left() as f64 + (bounding_box.width() - 1) as f64 / 2.0,
                bounding_box.top() as f64 + (bounding_box.height() - 1) as f64 / 2.0);
    }
    (weighted_x / weight_sum, weighted_y / weight_sum)
}

/// Estimates the RMS noise of the given image. A small portion of the image
/// is processed as follows:
/// 1. The brightest pixels are excluded.
/// 2. The standard deviation of the remaining pixels is computed.
///
/// To guard against accidentally sampling a bright part of the image (moon?
/// streetlamp?), we sample a few image regions and choose the darkest one to
/// measure the noise.
pub fn estimate_noise_from_image(image: &GrayImage) -> f64 {
    let noise_start = Instant::now();
    let (width, height) = image.dimensions();

    // The IMX296mono camera on Raspberry Pi Zero 2 W has a noise problem that
    // causes some rows to differ in offset. We thus use horizontal cuts within
    // a row when estimating noise, to avoid possible between-row level
    // variations.
    let cut_size = cmp::min(50, width / 4);

    // Sample three areas across each of five horizontal cuts of the image.
    let mut stats_arr = Vec::with_capacity(15);
    for vert in 1..=5 {
        let y = (height * vert / 6) as i32;
        for horiz in 1..=3 {
            let x = (width * horiz / 4 - cut_size/2) as i32;
            stats_arr.push(
                stats_for_roi(image, &Rect::at(x, y).of_size(cut_size, 1)));
        }
    }
    // Pick the darkest cut by mean value.
    stats_arr.sort_by(|a, b| compare_floats(&a.mean, &b.mean));
    let stddev = stats_arr[0].stddev;
    debug!("Noise estimate {} found in {:?}", stddev, noise_start.elapsed());
    stddev
}

/// Estimates the background and noise level of the given image region.
pub fn estimate_background_from_image_region(image: &GrayImage, roi: &Rect)
                                             -> (f64, f64) {
    let stats = stats_for_roi(image, roi);
    (stats.mean, stats.stddev)
}

// Returns mean/median/stddev for the given image region. Excludes bright
// pixels that are likely to be stars, as we're interested in the statistics
// of the sky background.
fn stats_for_roi(image: &GrayImage, roi: &Rect) -> HistogramStats {
    let mut histogram = [0_u32; 256];
    for_each_pixel_in_roi(image, roi, |_x, _y, pixel_value| {
        histogram[pixel_value as usize] += 1;
    });
    remove_stars_from_histogram(&mut histogram, /*sigma=*/8.0);
    stats_for_histogram(&histogram)
}

/// This function runs the CedarDetect algorithm on the supplied `image`, returning
/// a [StarDescription] for each detected star.
///
/// # Arguments
///   `image` - The image to analyze. The entire image is scanned for stars,
///   excluding the three leftmost and three rightmost columns.
///
///   `binned_image` - If provided, this is the 2x2 binning of `image`,
///   supplied as an `Arc` to avoid copying.
///
///   `noise_estimate` The noise level of `image`. This is typically the noise
///   level returned by [estimate_noise_from_image()].
///
///   `sigma` - Specifies the statistical significance threshold used for
///   discriminating stars from background. Given a noise measure N, a pixel's
///   value must be at least `sigma`*N greater than the background value in order
///   to be considered a star candidate. Higher `sigma` values yield fewer
///   stars; lower `sigma` values yield more stars but increase the likelihood
///   of noise-induced false positives. Typical `sigma` values: 5-10.
///
///   `binning` 1 (no binning), 2 (2x2 binning), 4 (4x4 binning), or 8 (8x8
///   binning). Specifies whether `image` should be additionally binned inside
///   this function prior to star detection. Note that the centroids of detected
///   stars is always reported in the coordinate space of the input `image`.
///
/// # Returns
/// Vec<[StarDescription]>, in order of descending estimated brightness.
///
/// Vec<[HotPixel]>: the isolated hot pixels detected, each with its image
///   coordinates and interpolated replacement value.
///
/// Option<Arc<GrayImage>>: the 2x2 binning of `image` when `binning` > 1,
///   otherwise None. This will be `binned_image` if provided, otherwise a
///   freshly computed binned image.
pub fn get_stars_from_image(image: &GrayImage,
                            binned_image: Option<Arc<GrayImage>>,
                            noise_estimate: f64,
                            sigma: f64,
                            binning: u32)
                            -> (Vec<StarDescription>,
                                Vec<HotPixel>,
                                Option<Arc<GrayImage>>)
{
    match binning {
        1 | 2 | 4 | 8 => (),
        _ => {
            panic!("Invalid binning argument {}, must be 1, 2, 4, or 8", binning);
        }
    }

    let start = Instant::now();

    // If noise estimate is below noise_floor, assume that the image background
    // has been crushed to black; use a minimum noise value.
    let noise_floor = 0.2;
    let noise_estimate = f64::max(noise_estimate, noise_floor);
    let sigma_noise_2 = cmp::max((2.0 * sigma * noise_estimate + 0.5) as i16, 2);
    // Both are measured against the full resolution image, which is what
    // all_bright_are_hot() examines regardless of binning.
    let min_neighbor_excess_8 = min_neighbor_excess_8(noise_estimate);

    let mut stars = Vec::<StarDescription>::new();
    let mut hot_pixels = Vec::<HotPixel>::new();

    // CedarDetect clumps adjacent bright pixels to form a single star
    // candidate. The `max_size` value governs how large a clump can be before
    // it is rejected. Note that making `max_size` small can eliminate very
    // bright stars that "bleed" to many surrounding pixels.
    let max_size = image.dimensions().0 / 100;

    if binning == 1 {
        let candidates_1d =
            scan_image_for_candidates(image, noise_estimate, sigma);
        let mut filtered_candidates = Vec::<CandidateFrom1D>::new();
        let mut max_y = 0usize;
        for cand in candidates_1d {
            if all_bright_are_hot(image, cand.x, cand.y, binning,
                                  sigma_noise_2, min_neighbor_excess_8,
                                  &mut hot_pixels) {
                // Discard candidate.
            } else {
                max_y = max_y.max(cand.y as usize);
                filtered_candidates.push(cand);
            }
        }
        for blob in form_blobs_from_candidates(filtered_candidates, max_y) {
            if let Some(x) = gate_star_2d(&blob, image,
                               /*higher_res_image=*/image,
                               binning, noise_estimate,
                               /*higher_res_noise_estimate=*/noise_estimate,
                               sigma, max_size, max_size) {
                stars.push(x)
            }
        }

        // Sort by brightness estimate, brightest first.
        stars.sort_by(|a, b| compare_floats(&b.brightness, &a.brightness));

        debug!("Star detection completed in {:?}", start.elapsed());
        return (stars, hot_pixels, None);
    }

    // We are binning by 2x, 4x, or 8x.
    let binned_2x: Arc<GrayImage> =
        binned_image.unwrap_or_else(|| Arc::new(bin_2x2(image)));
    // higher_res_image: one binning level below detect_image, used for
    // centroiding.
    let higher_res_image: &GrayImage;
    let detect_image: &GrayImage;
    // Noise level of `higher_res_image`, needed for centroiding. It is
    // higher than the detection image's noise, as `higher_res_image` is one
    // binning level less.
    let higher_res_noise_estimate: f64;
    let binned_4x;   // must outlive borrows for binning==4/8
    let binned_8x;   // must outlive borrow for binning==8
    if binning == 2 {
        detect_image = &binned_2x;
        higher_res_image = image;
        // higher_res_image is the original image, whose noise level the
        // caller already gave us.
        higher_res_noise_estimate = noise_estimate;
    } else {
        binned_4x = bin_2x2(&binned_2x);
        if binning == 4 {
            detect_image = &binned_4x;
            higher_res_image = &binned_2x;
        } else {
            // binning == 8
            binned_8x = bin_2x2(&binned_4x);
            detect_image = &binned_8x;
            higher_res_image = &binned_4x;
        }
        higher_res_noise_estimate =
            f64::max(estimate_noise_from_image(higher_res_image), noise_floor);
    }
    let noise_estimate_binned =
        f64::max(estimate_noise_from_image(&detect_image), noise_floor);

    debug!("Image preprocessing completed in {:?}", start.elapsed());
    let detect_start = Instant::now();

    let candidates_1d =
        scan_image_for_candidates(&detect_image, noise_estimate_binned, sigma);
    let mut filtered_candidates = Vec::<CandidateFrom1D>::new();
    let mut max_y = 0usize;
    for cand in candidates_1d {
        if all_bright_are_hot(image, cand.x, cand.y, binning,
                              sigma_noise_2, min_neighbor_excess_8,
                              &mut hot_pixels) {
            // candidate discarded; hot pixels already collected above
        } else {
            max_y = max_y.max(cand.y as usize);
            filtered_candidates.push(cand);
        }
    }
    for blob in form_blobs_from_candidates(filtered_candidates, max_y) {
        if let Some(x) = gate_star_2d(&blob, &detect_image,
                           &higher_res_image,
                           binning, noise_estimate_binned,
                           higher_res_noise_estimate, sigma,
                           max_size/binning + 1, max_size/binning + 1) { stars.push(x) }
    }

    // Sort by brightness estimate, brightest first.
    stars.sort_by(|a, b| compare_floats(&b.brightness, &a.brightness));

    debug!("Star detection completed in {:?}", detect_start.elapsed());
    (stars, hot_pixels, Some(binned_2x))
}

// Given a star candidate in a (possibly) binned image, see if any pixel(s) in
// the full resolution image contribute to the star candidate are non-hot
// bright. Pushes any hot sub-pixels into `hot_pixels`.
fn all_bright_are_hot(full_res_image: &GrayImage,
                      x: i32, y: i32, binning: u32,
                      sigma_noise_2: i16, min_neighbor_excess_8: i16,
                      hot_pixels: &mut Vec<HotPixel>) -> bool {
    let width = full_res_image.dimensions().0 as i32;
    let image_pixels: &Vec<u8> = full_res_image.as_raw();

    // Translate coordinates to full resolution.
    let x_full = x * binning as i32;
    let y_full = y * binning as i32;

    let mut has_bright = false;
    let mut has_hot = false;
    // Process all of the pixels in the full_res_image that contribute
    // to the (binned) x/y pixel.
    for yi in 0..binning {
        let backing_y = y_full + yi as i32;
        let row_pixels: &[u8] = &image_pixels.as_slice()
            [(backing_y * width) as usize ..
             ((backing_y+1) * width) as usize];
        // The rows bracketing this one, for classify_pixel()'s vertical
        // neighbors. scan_image_for_candidates() skips the outermost rows, so
        // these exist for any candidate we are called with.
        let above_pixels: &[u8] = &image_pixels.as_slice()
            [((backing_y - 1) * width) as usize ..
             (backing_y * width) as usize];
        let below_pixels: &[u8] = &image_pixels.as_slice()
            [((backing_y + 1) * width) as usize ..
             ((backing_y + 2) * width) as usize];

        for xi in 0..binning {
            let backing_x = x_full + xi as i32;

            let start_x = (backing_x - 3) as usize;
            let end_x = (backing_x + 4) as usize;
            let gate = &row_pixels[start_x .. end_x];
            let gate_above = &above_pixels[start_x .. end_x];
            let gate_below = &below_pixels[start_x .. end_x];
            let (pixel_type, val) =
                classify_pixel(gate_above, gate, gate_below, sigma_noise_2,
                               min_neighbor_excess_8);
            match pixel_type {
                PixelHotType::Bright => { has_bright = true; }
                PixelHotType::Hot => {
                    has_hot = true;
                    hot_pixels.push(HotPixel {
                        x: backing_x as u32,
                        y: backing_y as u32,
                        replacement_value: val,
                    });
                }
                PixelHotType::Dark => {}
            }
        }
    }

    has_hot && !has_bright
}

#[derive(Debug, Eq, PartialEq)]
enum PixelHotType {
    Dark,  // Pixel is not bright compared to threshold.
    Bright,  // Pixel is bright compared to threshold but is not isolated (hot).
    Hot,  // Pixel is bright compared to threshold and is isolated (hot).
}

// Returns Dark if the center pixel is not bright, so is neither part of a
// star nor an isolated hot pixel.
// Returns Bright if the center pixel is bright enough to possibly be part
// of a star.
// Returns Hot if the center of the gate is an isolated hot pixel. In this
// case the pixel value is replaced with the average of its neighbors.
//
// `gate_above` and `gate_below` are the 7-pixel gates from the rows directly
// above and below `gate`, aligned to the same columns. Callers guarantee those
// rows exist; scan_image_for_candidates() skips the outermost rows, which
// cannot yield stars anyway.
//
// Each row's own border and margin pixels estimate that row's background, so
// the vertical neighbors are compared against their own rows rather than the
// center's (sensors such as the IMX296mono vary in offset from row to row).
//
// `min_neighbor_excess_8` is 8x the smallest summed neighbor excess we are
// willing to treat as evidence of a star, in pixel value units. It floors the
// isolation threshold: without it, a candidate whose center barely clears the
// brightness gate gets an isolation threshold smaller than the noise on the
// neighbor sum, making the comparison meaningless. See
// min_neighbor_excess_8() for how it is derived.
fn classify_pixel(gate_above: &[u8], gate: &[u8], gate_below: &[u8],
                  sigma_noise_2: i16, min_neighbor_excess_8: i16)
                  -> (PixelHotType, u8)
{
    let lb = gate[0] as i16;
    let c = gate[3] as i16;
    let rb = gate[6] as i16;

    // Hot pixel must be bright compared to background. This test stays on the
    // two border pixels, matching gate_star_1d()'s notion of background so the
    // two agree about which pixels are worth considering at all.
    let est_background_2 = lb + rb;
    let center_minus_background_2 = c + c - est_background_2;
    if center_minus_background_2 < sigma_noise_2 {
        return (PixelHotType::Dark, gate[3]);
    }

    // Sum of l + r + u + d (each minus its own row's background) must exceed
    // 0.5 * center (minus background). Otherwise, the center is a hot pixel.
    //
    // With twice as many neighbors, the 0.5 ratio admits the same point spread
    // that 0.25 admits for two neighbors, so widening the gate improves the
    // noise margin rather than changing which stars qualify.
    //
    // A row's background sums its border and margin pixels, at +/-3 and +/-2
    // from center; both are far enough out that a single hot pixel does not
    // reach them. Sums are kept scaled by 4 (four background pixels per row) to
    // stay in integer arithmetic.
    // Pixel labels follow gate_star_1d()'s |lb lm l C r rm rb|, with u and d
    // the vertical neighbors drawn from the rows above and below.
    let lm = gate[1] as i16;
    let l = gate[2] as i16;
    let r = gate[4] as i16;
    let rm = gate[5] as i16;
    let u = gate_above[3] as i16;
    let d = gate_below[3] as i16;

    // A row's background is the sum of its four outer pixels, lb + lm + rm + rb,
    // which is 4x that row's mean. Everything below therefore carries a factor
    // of 4, which is what keeps the arithmetic in integers.
    let row_background_4 = |g: &[u8]| -> i16 {
        g[0] as i16 + g[1] as i16 + g[5] as i16 + g[6] as i16
    };
    let background_center_4 = est_background_2 + lm + rm;
    let background_above_4 = row_background_4(gate_above);
    let background_below_4 = row_background_4(gate_below);

    // 4x a pixel's excess over the background of whichever row it sits in.
    let excess_4 = |pixel: i16, background_4: i16| 4 * pixel - background_4;

    // Each neighbor is measured against its own row. A row sitting at a
    // different offset than its neighbors then cancels out of the sum rather
    // than masquerading as neighbor flux.
    let neighbor_excess_4 = excess_4(l, background_center_4) +
                            excess_4(r, background_center_4) +
                            excess_4(u, background_above_4) +
                            excess_4(d, background_below_4);
    let center_excess_4 = excess_4(c, background_center_4);

    // The neighbors must carry more than half the center's excess, and in any
    // case at least min_neighbor_excess. Comparing at 8x lets the halving fall
    // out for free: half of center_excess_4 at 8x scale is center_excess_4.
    let threshold_8 = cmp::max(center_excess_4, min_neighbor_excess_8);
    if 2 * neighbor_excess_4 <= threshold_8 {
        let neighbor_sum = l + r + u + d;
        return (PixelHotType::Hot, (neighbor_sum / 4) as u8);
    }
    (PixelHotType::Bright, gate[3])
}

// The isolation test in classify_pixel() asks whether a candidate's neighbors
// carry enough flux to be a star rather than a lone hot pixel. Its threshold
// is half the center pixel's excess over background, which shrinks as
// candidates get fainter; below roughly `sigma` 7 it drops under the noise of
// the neighbor sum itself and the test loses its power. This returns a floor
// for that threshold, so a marginal candidate must still show neighbor flux
// that is significantly higher than background, rather than merely a fraction
// of a faint center.
//
// The multiplier is stated in units of the neighbor sum's own standard
// deviation, which is sqrt(5.5) ~= 2.35 times the per-pixel noise: four
// neighbors, each referenced to a four-pixel row background. A floor of ~1.5
// of those standard deviations roughly equalizes the two error rates (leaking
// a hot pixel versus discarding a faint star) at low `sigma`, and sits below
// the ratio threshold at `sigma` 8 and above, leaving normal operation
// untouched.
//
// Returned pre-scaled by 8 to match classify_pixel()'s integer arithmetic.
fn min_neighbor_excess_8(noise_estimate: f64) -> i16 {
    const NEIGHBOR_SUM_STDDEV_PER_NOISE: f64 = 2.345;
    const FLOOR_STDDEVS: f64 = 1.5;
    (8.0 * FLOOR_STDDEVS * NEIGHBOR_SUM_STDDEV_PER_NOISE * noise_estimate + 0.5)
        as i16
}

/// Summarizes an image region of interest. The pixel values used are not
/// background subtracted. Single hot pixels are rejected when locating the peak
/// pixel; the histogram accumulates the original pixel values.
#[derive(Debug)]
pub struct RegionOfInterestSummary {
    /// Histogram of raw pixel values in the ROI. Hot pixels are counted at
    /// their own values; only `peak_x`/`peak_y` exclude them.
    pub histogram: [u32; 256],

    /// The location (in image coordinates) of the peak pixel. If there are
    /// multiple pixels with the peak value, the one closest to the image
    /// center is reported here. The application logic should use `histogram`
    /// to adjust exposure to avoid too many peak (saturated) pixels.
    pub peak_x: f64,
    pub peak_y: f64,

    /// Mean of the pixel values in a 3x3 pixel square centered on the peak
    /// pixel. Not dark subtracted.
    pub peak_value: f64,  // 0..255.
}

/// Gathers information from a region of interest of an image.
///
/// # Arguments
///   `image` - The image of which a portion will be summarized.
///   `roi` - Specifies the portion of `image` to be summarized.
///   `noise_estimate` The noise level of `image`. This is typically the noise
///       level returned by [estimate_noise_from_image()].
///   `sigma` - Specifies the statistical significance threshold used for
///       discriminating hot pixels from background. This can be the same value
///       as passed to [get_stars_from_image()].
///
/// # Returns
/// [RegionOfInterestSummary] Information that compactly provides some information
/// about the ROI. The `histogram` result can be used by an application's
/// auto-exposure logic; the `peak_x` and `peak_y` result can be used to identify
/// a target for focusing.
///
/// # Panics
/// The `roi` must exclude the three leftmost and three rightmost image columns.
/// The `roi` must exclude the three top and three bottom image rows.
pub fn summarize_region_of_interest(image: &GrayImage, roi: &Rect,
                                    noise_estimate: f64, sigma: f64)
                                    -> RegionOfInterestSummary {
    let process_roi_start = Instant::now();
    let (width, height) = image.dimensions();
    // Compute the ROI center, in image coordinates.
    let (roi_center_x, roi_center_y) =
        (roi.left() + roi.width() as i32 / 2,
         roi.top() + roi.height() as i32 / 2);
    let sigma_noise_2 = cmp::max((2.0 * sigma * noise_estimate + 0.5) as i16, 2);
    let min_neighbor_excess_8 = min_neighbor_excess_8(noise_estimate);

    // Sliding gate needs to extend past left and right edges of ROI. Make sure
    // there's enough image.
    let gate_leftmost = roi.left() - 3;
    let gate_rightmost = roi.right() + 4;  // One past.
    assert!(gate_leftmost >= 0);
    assert!(gate_rightmost <= width as i32);

    // We also need top/bottom margin to allow centroiding.
    let top = roi.top() - 3;
    let bottom = roi.bottom() + 4;  // One past.
    assert!(top >= 0);
    assert!(bottom <= height as i32);
    let image_pixels: &Vec<u8> = image.as_raw();

    let mut histogram = [0_u32; 256];
    let mut peak_x = 0;
    let mut peak_y = 0;
    let mut peak_val = 0_u8;
    let mut peak_center_dist_sq = i32::MAX;
    // The scan reads raw pixel values, histogramming each and testing it as a
    // peak candidate. The 7 pixel gate that identifies hot pixels is formed
    // only for candidates -- pixels reaching `peak_val`, which is a ratcheting
    // threshold, so this happens a handful of times over the whole ROI rather
    // than once per pixel.
    // Consequently the histogram holds raw values, including hot pixels.
    for rownum in roi.top()..=roi.bottom() {
        // Get the slice of image_pixels corresponding to this row of the ROI.
        let row_start = (rownum * width as i32) as usize;
        let row_pixels: &[u8] = &image_pixels.as_slice()
            [row_start + roi.left() as usize ..
             row_start + roi.right() as usize + 1];
        let mut center_x = roi.left();
        for &val in row_pixels {
            histogram[val as usize] += 1;
            if val >= peak_val {
                // Candidate peak, reject hot pixel.
                // The margins asserted above guarantee the bracketing rows
                // and the horizontal gate extent all exist.
                // The gate spans center_x-3 ..= center_x+3, in each of the
                // three rows.
                let gate_x = (center_x - 3) as usize;
                let above_start = ((rownum - 1) * width as i32) as usize;
                let below_start = ((rownum + 1) * width as i32) as usize;
                let pixels = image_pixels.as_slice();
                let gate: &[u8] =
                    &pixels[row_start + gate_x .. row_start + gate_x + 7];
                let gate_above: &[u8] =
                    &pixels[above_start + gate_x .. above_start + gate_x + 7];
                let gate_below: &[u8] =
                    &pixels[below_start + gate_x .. below_start + gate_x + 7];
                let (pixel_type, _classified_val) =
                    classify_pixel(gate_above, gate, gate_below, sigma_noise_2,
                                   min_neighbor_excess_8);
                // A hot pixel cannot be the peak; ignore it entirely, leaving
                // `peak_val` untouched so later stars remain eligible.
                if pixel_type != PixelHotType::Hot {
                    let center_dist_sq =
                        (center_x - roi_center_x) * (center_x - roi_center_x) +
                        (rownum - roi_center_y) * (rownum - roi_center_y);

                    let mut new_peak = false;
                    if val == peak_val {
                        // Break ties towards image center.
                        if center_dist_sq < peak_center_dist_sq {
                            new_peak = true;
                        }
                    } else {
                        new_peak = true;
                    }
                    if new_peak {
                        peak_x = center_x;
                        peak_y = rownum;
                        peak_val = val;
                        peak_center_dist_sq = center_dist_sq;
                    }
                }
            }  // Candidate peak.
            center_x += 1;
        }
    }

    // Apply centroiding to get sub-pixel resolution for peak_x/y.
    let bounding_box = Rect::at(peak_x - 2, peak_y - 2).of_size(5, 5);
    let (mut x, mut y) = compute_centroid(image, &bounding_box, noise_estimate);
    x += 0.5;
    y += 0.5;
    // Constrain x/y to be within ROI.
    x = cmp::max_by(x, roi.left() as f64, compare_floats);
    x = cmp::min_by(x, roi.right() as f64, compare_floats);
    y = cmp::max_by(y, roi.top() as f64, compare_floats);
    y = cmp::min_by(y, roi.bottom() as f64, compare_floats);

    let value_box = Rect::at(peak_x - 1, peak_y - 1).of_size(3, 3);
    let mut box_sum: i32 = 0;
    let mut box_count: i32 = 0;
    for_each_pixel_in_roi(image, &value_box, |_x, _y, pixel_value| {
        box_sum += pixel_value as i32;
        box_count += 1;
    });
    let peak_value = box_sum as f64 / box_count as f64;

    debug!("ROI processing completed in {:?}", process_roi_start.elapsed());
    RegionOfInterestSummary{histogram, peak_x: x, peak_y: y, peak_value}
}

#[cfg(test)]
mod tests {
    extern crate approx;
    use approx::assert_abs_diff_eq;
    use image::Luma;
    use imageproc::gray_image;
    use imageproc::noise::gaussian_noise;
    use std::time::Duration;
    use super::*;

    #[test]
    #[should_panic]
    fn test_for_each_pixel_in_roi_too_large() {
        let empty_image = gray_image!();
        for_each_pixel_in_roi(&empty_image, &Rect::at(0, 0).of_size(1, 1), |_x, _y, _p| {});
    }

    #[test]
    fn test_for_each_pixel_in_roi_1x1() {
        let image_1x1 = gray_image!(127);
        let mut pixels = Vec::new();
        for_each_pixel_in_roi(&image_1x1, &Rect::at(0, 0).of_size(1, 1), |x, y, p| {
            pixels.push((x, y, p));
        });
        assert_eq!(pixels, vec!((0, 0, 127)));
    }

    #[test]
    fn test_for_each_pixel_in_roi_3x3() {
        let image_3x3 = gray_image!(
            0, 1, 2;
            127, 253, 254;
            255, 0, 1);

        // Entire ROI, with interior.
        let mut pixels = Vec::new();
        for_each_pixel_in_roi(&image_3x3, &Rect::at(0, 0).of_size(3, 3), |x, y, p| {
            pixels.push((x, y, p));
        });
        assert_eq!(pixels, vec!((0, 0, 0),
                                (1, 0, 1),
                                (2, 0, 2),
                                (0, 1, 127),
                                (1, 1, 253),
                                (2, 1, 254),
                                (0, 2, 255),
                                (1, 2, 0),
                                (2, 2, 1)));

        // Entire ROI, no interior.
        let mut perimeter_pixels = Vec::new();
        for_each_perimeter_pixel(&image_3x3, &Rect::at(0, 0).of_size(3, 3), |x, y, p| {
            perimeter_pixels.push((x, y, p));
        });
        assert_eq!(perimeter_pixels, vec!((0, 0, 0),
                                          (1, 0, 1),
                                          (2, 0, 2),
                                          (0, 1, 127),
                                          (2, 1, 254),
                                          (0, 2, 255),
                                          (1, 2, 0),
                                          (2, 2, 1)));
    }

    #[test]
    fn test_stats_for_roi() {
        let mut image_5x4 = GrayImage::new(5, 4);
        // Leave image as all zeros for now.
        let mut stats = stats_for_roi(&image_5x4,
                                      &Rect::at(0, 0).of_size(5, 4));
        assert_eq!(stats.mean, 0_f64);
        assert_eq!(stats.median, 0);
        assert_eq!(stats.stddev, 0_f64);

        // Add some noise.
        image_5x4.put_pixel(1, 0, Luma::<u8>([1]));
        image_5x4.put_pixel(2, 0, Luma::<u8>([2]));
        image_5x4.put_pixel(3, 0, Luma::<u8>([1]));
        image_5x4.put_pixel(4, 0, Luma::<u8>([2]));
        image_5x4.put_pixel(0, 1, Luma::<u8>([1]));
        stats = stats_for_roi(&image_5x4,
                              &Rect::at(0, 0).of_size(5, 4));
        assert_eq!(stats.mean, 0.35);
        assert_eq!(stats.median, 0);
        assert_abs_diff_eq!(stats.stddev, 0.65, epsilon = 0.01);

        // Single bright pixel. This is removed because we eliminate the
        // brightest pixel(s).
        image_5x4.put_pixel(0, 0, Luma::<u8>([255]));
        stats = stats_for_roi(&image_5x4,
                              &Rect::at(0, 0).of_size(5, 4));
        assert_abs_diff_eq!(stats.mean, 0.368, epsilon = 0.001);
        assert_eq!(stats.median, 0);
        assert_abs_diff_eq!(stats.stddev, 0.66, epsilon = 0.01);

        // Add another non-zero pixel. This will be kept.
        image_5x4.put_pixel(0, 1, Luma::<u8>([4]));
        stats = stats_for_roi(&image_5x4,
                              &Rect::at(0, 0).of_size(5, 4));
        assert_abs_diff_eq!(stats.mean, 0.526, epsilon = 0.001);
        assert_eq!(stats.median, 0);
        assert_abs_diff_eq!(stats.stddev, 1.04, epsilon = 0.01);
    }

    #[test]
    fn test_estimate_noise_from_image() {
        let small_image = gaussian_noise(&GrayImage::new(100, 100),
                                         100.0, 3.0, 42);
        // The stddev is what we requested because there are no bright
        // outliers to discard.
        assert_abs_diff_eq!(estimate_noise_from_image(&small_image),
                            3.0, epsilon = 0.5);
        let large_image = gaussian_noise(&GrayImage::new(1000, 1000),
                                         100.0, 3.0, 42);
        assert_abs_diff_eq!(estimate_noise_from_image(&large_image),
                            3.0, epsilon = 0.5);
    }

    #[test]
    fn test_gate_star_1d_center_bright_wrt_background() {
        // Center minus background not bright enough.
        let mut gate: [u8; 7] = [10, 10, 10, 12, 10, 10, 10];
        let mut result_type =
            gate_star_1d(&gate, /*sigma_noise_2=*/5, /*sigma_noise_3=*/7);
        assert_eq!(result_type, ResultType::Uninteresting);

        // Center minus background is bright enough.
        gate = [10, 10, 11, 13, 11, 10, 10];
        result_type =
            gate_star_1d(&gate, /*sigma_noise_2=*/5, /*sigma_noise_3=*/7);
        assert_eq!(result_type, ResultType::Candidate);
    }

    #[test]
    fn test_gate_star_1d_center_bright_wrt_neighbor() {
        // Center is less than a neighbor.
        let mut gate: [u8; 7] = [10, 10, 11, 13, 14, 10, 10];
        let mut result_type =
            gate_star_1d(&gate, /*sigma_noise_2=*/5, /*sigma_noise_3=*/7);
        assert_eq!(result_type, ResultType::Uninteresting);

        // Ditto, other neighbor.
        gate = [10, 10, 14, 13, 11, 10, 10];
        result_type =
            gate_star_1d(&gate, /*sigma_noise_2=*/5, /*sigma_noise_3=*/7);
        assert_eq!(result_type, ResultType::Uninteresting);

        // Center is at least as bright as its neighbors. Tie break is to
        // left (current candidate); this is explored further below.
        gate = [10, 10, 11, 13, 13, 10, 10];
        result_type =
            gate_star_1d(&gate, /*sigma_noise_2=*/5, /*sigma_noise_3=*/7);
        assert_eq!(result_type, ResultType::Candidate);
    }

    #[test]
    fn test_gate_star_1d_center_bright_wrt_margin() {
        // Center is not brighter than a margin.
        let mut gate: [u8; 7] = [10, 10, 11, 13, 11, 13, 10];
        let mut result_type =
            gate_star_1d(&gate, /*sigma_noise_2=*/5, /*sigma_noise_3=*/7);
        assert_eq!(result_type, ResultType::Uninteresting);

        // Ditto, other margin.
        gate = [10, 13, 11, 13, 11, 10, 10];
        result_type =
            gate_star_1d(&gate, /*sigma_noise_2=*/5, /*sigma_noise_3=*/7);
        assert_eq!(result_type, ResultType::Uninteresting);

        // Center brighter than both margins.
        gate = [10, 12, 11, 13, 11, 12, 10];
        result_type =
            gate_star_1d(&gate, /*sigma_noise_2=*/5, /*sigma_noise_3=*/7);
        assert_eq!(result_type, ResultType::Candidate);
    }

    #[test]
    fn test_gate_star_1d_left_center_tie() {
        // When center and left have same value, tie is broken
        // based on next surrounding values.
        // In this case, the tie breaks to the left.
        let mut gate: [u8; 7] = [10, 11, 13, 13, 10, 11, 10];
        let mut result_type =
            gate_star_1d(&gate, /*sigma_noise_2=*/5, /*sigma_noise_3=*/7);
        assert_eq!(result_type, ResultType::Uninteresting);

        // Here, the tie breaks to the right (center pixel).
        gate = [10, 11, 13, 13, 11, 11, 10];
        result_type =
            gate_star_1d(&gate, /*sigma_noise_2=*/5, /*sigma_noise_3=*/7);
        assert_eq!(result_type, ResultType::Candidate);
    }

    #[test]
    fn test_gate_star_1d_center_right_tie() {
        // When center and right have same value, tie is broken
        // based on next surrounding values.
        // In this case, the tie breaks to the right.
        let mut gate: [u8; 7] = [10, 11, 11, 13, 13, 11, 10];
        let mut result_type =
            gate_star_1d(&gate, /*sigma_noise_2=*/5, /*sigma_noise_3=*/7);
        assert_eq!(result_type, ResultType::Uninteresting);

        // Here, the tie breaks to the left (center pixel).
        gate = [10, 11, 11, 13, 13, 10, 10];
        result_type =
            gate_star_1d(&gate, /*sigma_noise_2=*/5, /*sigma_noise_3=*/7);
        assert_eq!(result_type, ResultType::Candidate);
    }

    #[test]
    fn test_gate_star_1d_unequal_border() {
        // Border pixels differ too much, so candidate is rejected.
        let mut gate: [u8; 7] = [15, 10, 12, 18, 13, 10, 4];
        let mut result_type =
            gate_star_1d(&gate, /*sigma_noise_2=*/7, /*sigma_noise_3=*/9);
        assert_eq!(result_type, ResultType::Uninteresting);

        // Borders are close enough now.
        gate = [13, 10, 12, 18, 13, 10, 6];
        result_type =
            gate_star_1d(&gate, /*sigma_noise_2=*/7, /*sigma_noise_3=*/9);
        assert_eq!(result_type, ResultType::Candidate);
    }

    #[test]
    fn test_gate_star_1d_three_bright() {
        // Three equally bright pixels, left pixel.
        let mut gate: [u8; 7] = [10, 10, 10, 18, 18, 18, 10];
        let mut result_type =
            gate_star_1d(&gate, /*sigma_noise_2=*/7, /*sigma_noise_3=*/9);
        assert_eq!(result_type, ResultType::Uninteresting);

        // Middle pixel.
        gate = [10, 10, 18, 18, 18, 10, 10];
        result_type =
            gate_star_1d(&gate, /*sigma_noise_2=*/7, /*sigma_noise_3=*/9);
        assert_eq!(result_type, ResultType::Candidate);

        // Right pixel.
        gate = [10, 18, 18, 18, 10, 10, 10];
        result_type =
            gate_star_1d(&gate, /*sigma_noise_2=*/7, /*sigma_noise_3=*/9);
        assert_eq!(result_type, ResultType::Uninteresting);
    }

    #[test]
    fn test_form_blobs_from_candidates() {
        let mut candidates = vec![
            CandidateFrom1D{x: 20, y:3},
            CandidateFrom1D{x: 23, y:3}
        ];
        // Candidates on the same row are not combined, even if they are close
        // together. This is because, in practice due to the operation of
        // gate_star_1d(), candidates in the same row will always be well
        // separated.
        let max_y = candidates.iter().map(|c| c.y as usize).max().unwrap_or(0);
        let mut blobs = form_blobs_from_candidates(candidates, max_y);
        blobs.sort_by(|a, b| a.candidates[0].x.cmp(&b.candidates[0].x));
        assert_eq!(blobs.len(), 2);
        assert_eq!(blobs[0].candidates.len(), 1);
        assert_eq!(blobs[0].candidates[0].x, 20);
        assert_eq!(blobs[0].candidates[0].y, 3);
        assert_eq!(blobs[1].candidates.len(), 1);
        assert_eq!(blobs[1].candidates[0].x, 23);
        assert_eq!(blobs[1].candidates[0].y, 3);

        // Candidates on adjacent rows are combined if they are close
        // enough w.r.t. their horizontal offset.
        // Not combined:
        // . . . . . A . . . . . .
        // . B . . . . . . . . . .
        candidates = vec![
            CandidateFrom1D{x: 5, y:0},  // A.
            CandidateFrom1D{x: 1, y:1}   // B.
        ];
        let max_y = candidates.iter().map(|c| c.y as usize).max().unwrap_or(0);
        blobs = form_blobs_from_candidates(candidates, max_y);
        assert_eq!(blobs.len(), 2);

        // Combined:
        // . . . . A . . . . . . .
        // . B . . . . . . . . . .
        candidates = vec![
            CandidateFrom1D{x: 4, y:0},  // A.
            CandidateFrom1D{x: 1, y:1}   // B.
        ];
        let max_y = candidates.iter().map(|c| c.y as usize).max().unwrap_or(0);
        blobs = form_blobs_from_candidates(candidates, max_y);
        assert_eq!(blobs.len(), 1);

        // Combined:
        // . . . . A . . . . . . .
        // . . . . B . . . . . . .
        candidates = vec![
            CandidateFrom1D{x: 4, y:0},  // A.
            CandidateFrom1D{x: 4, y:1}   // B.
        ];
        let max_y = candidates.iter().map(|c| c.y as usize).max().unwrap_or(0);
        blobs = form_blobs_from_candidates(candidates, max_y);
        assert_eq!(blobs.len(), 1);

        // Combined:
        // . . . . A . . . . . . .
        // . . . . . . . B . . . .
        candidates = vec![
            CandidateFrom1D{x: 4, y:0},  // A.
            CandidateFrom1D{x: 7, y:1}   // B.
        ];
        let max_y = candidates.iter().map(|c| c.y as usize).max().unwrap_or(0);
        blobs = form_blobs_from_candidates(candidates, max_y);
        assert_eq!(blobs.len(), 1);

        // Not combined:
        // . . . . A . . . . . . .
        // . . . . . . . . B . . .
        candidates = vec![
            CandidateFrom1D{x: 4, y:0},  // A.
            CandidateFrom1D{x: 8, y:1}   // B.
        ];
        let max_y = candidates.iter().map(|c| c.y as usize).max().unwrap_or(0);
        blobs = form_blobs_from_candidates(candidates, max_y);
        assert_eq!(blobs.len(), 2);

        // In this case, B absorbs A and C. When D is reached, it sees that C
        // has been absorbed into B and D thus absorbs B (and its already
        // absorbed A and C).
        // . A . . . C . . . . . .
        // . . . B . . . D . . . .
        candidates = vec![
            CandidateFrom1D{x: 1, y:0},  // A.
            CandidateFrom1D{x: 3, y:1},  // B.
            CandidateFrom1D{x: 5, y:0},  // C.
            CandidateFrom1D{x: 7, y:1}   // D.
        ];
        let max_y = candidates.iter().map(|c| c.y as usize).max().unwrap_or(0);
        blobs = form_blobs_from_candidates(candidates, max_y);
        assert_eq!(blobs.len(), 1);
        assert_eq!(blobs[0].candidates.len(), 4);
    }

    #[test]
    fn test_gate_star_2d_too_large() {
        let mut blob = Blob{candidates: Vec::<CandidateFrom1D>::new(),
                            recipient_blob: None};
        let image_9x9 = gray_image!(
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9, 11, 11, 11, 11, 11,  9,  9;
            9,  9, 11, 18, 20, 19, 11,  9,  9;
            9,  9, 11, 20, 30, 20, 11,  9,  9;
            9,  9, 11, 19, 20, 19, 11,  9,  9;
            9,  9, 11, 11, 11, 11, 11,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9);
        // Make 3x3 blob.
        blob.candidates.push(CandidateFrom1D{x: 3, y: 3});
        blob.candidates.push(CandidateFrom1D{x: 5, y: 5});
        // 3x3 is too big.
        match gate_star_2d(&blob, &image_9x9, &image_9x9, 1, 1.0, 1.0, 6.0,
                           /*max_width=*/3, /*max_height=*/2) {
            Some(_star_description) => panic!("Expected rejection"),
            None => ()
        }
        match gate_star_2d(&blob, &image_9x9, &image_9x9, 1, 1.0, 1.0, 6.0,
                           /*max_width=*/2, /*max_height=*/3) {
            Some(_star_description) => panic!("Expected rejection"),
            None => ()
        }
        // We allow a 3x3 blob here.
        match gate_star_2d(&blob, &image_9x9, &image_9x9, 1, 1.0, 1.0, 6.0,
                           /*max_width=*/3, /*max_height=*/3) {
            Some(_star_description) => (),
            None => panic!("Expected candidate")
        }
    }

    #[test]
    fn test_gate_star_2d_image_boundary() {
        let mut blob = Blob{candidates: Vec::<CandidateFrom1D>::new(),
                            recipient_blob: None};
        let image_7x7 = gray_image!(
        9,  9,  9,  9,  9,  9,  9;
        9, 10, 10, 10, 10, 10,  9;
        9, 10, 12, 15, 12, 10,  9;
        9, 10, 15, 30, 15, 10,  9;
        9, 10, 12, 15, 12, 10,  9;
        9, 10, 10, 10, 10, 10,  9;
        9,  9,  9,  9,  9,  9,  9);
        // Make 1x1 blob, but too far to the left.
        blob.candidates.push(CandidateFrom1D{x: 2, y: 3});
        if let Some(_star_description) = gate_star_2d(&blob, &image_7x7, &image_7x7, 1, 1.0, 1.0, 6.0, 3, 3) { panic!("Expected rejection") }
        // Too far to right.
        blob.candidates.clear();
        blob.candidates.push(CandidateFrom1D{x: 4, y: 3});
        if let Some(_star_description) = gate_star_2d(&blob, &image_7x7, &image_7x7, 1, 1.0, 1.0, 6.0, 3, 3) { panic!("Expected rejection") }
        // Too high.
        blob.candidates.clear();
        blob.candidates.push(CandidateFrom1D{x: 3, y: 2});
        if let Some(_star_description) = gate_star_2d(&blob, &image_7x7, &image_7x7, 1, 1.0, 1.0, 6.0, 3, 3) { panic!("Expected rejection") }
        // Too low.
        blob.candidates.clear();
        blob.candidates.push(CandidateFrom1D{x: 3, y: 4});
        if let Some(_star_description) = gate_star_2d(&blob, &image_7x7, &image_7x7, 1, 1.0, 1.0, 6.0, 3, 3) { panic!("Expected rejection") }
        // Just right!
        blob.candidates.clear();
        blob.candidates.push(CandidateFrom1D{x: 3, y: 3});
        match gate_star_2d(&blob, &image_7x7, &image_7x7, 1, 1.0, 1.0, 6.0, 3, 3) {
            Some(_star_description) => (),
            None => panic!("Expected candidate"),
        }
    }

    #[test]
    fn test_gate_star_2d_hollow_core() {
        let mut blob = Blob{candidates: Vec::<CandidateFrom1D>::new(),
                            recipient_blob: None};
        let mut image_9x9 = gray_image!(
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9, 11, 11, 11, 11, 11,  9,  9;
            9,  9, 11, 19, 20, 19, 11,  9,  9;
            9,  9, 11, 20, 19, 20, 11,  9,  9;
            9,  9, 11, 19, 20, 19, 11,  9,  9;
            9,  9, 11, 11, 11, 11, 11,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9);
        // Make 3x3 blob.
        blob.candidates.push(CandidateFrom1D{x: 3, y: 3});
        blob.candidates.push(CandidateFrom1D{x: 5, y: 5});
        // Center of core is less bright than outer core.
        if let Some(_star_description) = gate_star_2d(&blob, &image_9x9, &image_9x9, 1, 1.0, 1.0, 6.0, 3, 3) { panic!("Expected rejection") }
        // Make center bright enough.
        image_9x9.put_pixel(4, 4, Luma::<u8>([20]));
        match gate_star_2d(&blob, &image_9x9, &image_9x9, 1, 1.0, 1.0, 6.0, 3, 3) {
            Some(_star_description) => (),
            None => panic!("Expected candidate"),
        }
    }

    #[test]
    fn test_gate_star_2d_core_bright_wrt_neighbor() {
        let mut blob = Blob{candidates: Vec::<CandidateFrom1D>::new(),
                            recipient_blob: None};
        blob.candidates.push(CandidateFrom1D{x: 3, y: 3});
        let mut image_7x7 = gray_image!(
        8,  8,  8,  8,  8,  8,  8;
        8, 10, 10, 10, 10, 10,  8;
        8, 10, 12, 14, 12, 10,  8;
        8, 10, 14, 13, 14, 10,  8;
        8, 10, 12, 14, 12, 10,  8;
        8, 10, 10, 10, 10, 10,  8;
        8,  8,  8,  8,  8,  8,  8);
        // 1x1 core is less bright than neighbor average. Note that the
        // neighbor corners are excluded.
        if let Some(_star_description) = gate_star_2d(&blob, &image_7x7, &image_7x7, 1, 1.0, 1.0, 6.0, 3, 3) { panic!("Expected rejection") }
        // Make center bright enough.
        image_7x7.put_pixel(3, 3, Luma::<u8>([14]));
        match gate_star_2d(&blob, &image_7x7, &image_7x7, 1, 1.0, 1.0, 6.0, 3, 3) {
            Some(_star_description) => (),
            None => panic!("Expected candidate"),
        }
    }

    #[test]
    fn test_gate_star_2d_core_bright_wrt_margin() {
        let mut blob = Blob{candidates: Vec::<CandidateFrom1D>::new(),
                            recipient_blob: None};
        blob.candidates.push(CandidateFrom1D{x: 3, y: 3});
        let mut image_7x7 = gray_image!(
        8,  8,  8,  8,  8,  8,  8;
        8, 14, 14, 14, 14, 14,  8;
        8, 14, 12, 14, 12, 14,  8;
        8, 14, 14, 14, 14, 14,  8;
        8, 14, 12, 14, 12, 14,  8;
        8, 14, 14, 14, 14, 14,  8;
        8,  8,  8,  8,  8,  8,  8);
        // 1x1 core is not brighter than margin average.
        if let Some(_star_description) = gate_star_2d(&blob, &image_7x7, &image_7x7, 1, 1.0, 1.0, 6.0, 3, 3) { panic!("Expected rejection") }
        // Make center bright enough.
        image_7x7.put_pixel(3, 3, Luma::<u8>([15]));
        match gate_star_2d(&blob, &image_7x7, &image_7x7, 1, 1.0, 1.0, 6.0, 3, 3) {
            Some(_star_description) => (),
            None => panic!("Expected candidate"),
        }
    }

    #[test]
    fn test_gate_star_2d_nonuniform_perimeter() {
        let mut blob = Blob{candidates: Vec::<CandidateFrom1D>::new(),
                            recipient_blob: None};
        blob.candidates.push(CandidateFrom1D{x: 3, y: 3});
        let mut image_7x7 = gray_image!(
        9,  9,  9,  9,  9,  9,  9;
        9, 10, 10, 10, 10, 10,  9;
       26, 10, 12, 15, 12, 10,  9;
        9, 10, 15, 30, 15, 10,  9;
        9, 10, 12, 15, 12, 10,  9;
        9, 10, 10, 10, 10, 10,  9;
        9,  9,  9,  9,  9,  9,  9);
        // Perimeter has an anomalously bright pixel.
        if let Some(_star_description) = gate_star_2d(&blob, &image_7x7, &image_7x7, 1, 1.0, 1.0, 6.0, 3, 3) { panic!("Expected rejection") }
        // Repair the perimeter.
        image_7x7.put_pixel(0, 2, Luma::<u8>([12]));
        match gate_star_2d(&blob, &image_7x7, &image_7x7, 1, 1.0, 1.0, 6.0, 3, 3) {
            Some(_star_description) => (),
            None => panic!("Expected candidate"),
        }
    }

    #[test]
    fn test_gate_star_2d_core_bright_wrt_perimeter() {
        let mut blob = Blob{candidates: Vec::<CandidateFrom1D>::new(),
                            recipient_blob: None};
        blob.candidates.push(CandidateFrom1D{x: 3, y: 3});
        let mut image_7x7 = gray_image!(
        8,  8,  8,  8,  8,  8,  8;
        8,  9,  9,  9,  9,  9,  8;
        8,  9, 10, 11, 10,  9,  8;
        8,  9, 11, 13, 11,  9,  8;
        8,  9, 10, 11, 10,  9,  8;
        8,  9,  9,  9,  9,  9,  8;
        8,  8,  8,  8,  8,  8,  8);
        // 1x1 core is not brighter enough than perimeter average.
        if let Some(_star_description) = gate_star_2d(&blob, &image_7x7, &image_7x7, 1, 1.0, 1.0, 6.0, 3, 3) { panic!("Expected rejection") }
        // Make center bright enough.
        image_7x7.put_pixel(3, 3, Luma::<u8>([14]));
        match gate_star_2d(&blob, &image_7x7, &image_7x7, 1, 1.0, 1.0, 6.0, 3, 3) {
            Some(_star_description) => (),
            None => panic!("Expected candidate"),
        }
    }

    #[test]
    fn test_compute_centroid() {
        let bounding_box = Rect::at(1, 1).of_size(5, 5);

        // Symmetric star centered on pixel (3, 3). Centroid is that pixel's
        // index; callers add 0.5 to reach continuous coords.
        let image_7x7 = gray_image!(
            9,  9,  9,  9,  9,  9,  9;
            9, 10, 10, 10, 10, 10,  9;
            9, 10, 20, 40, 20, 10,  9;
            9, 10, 40, 80, 40, 10,  9;
            9, 10, 20, 40, 20, 10,  9;
            9, 10, 10, 10, 10, 10,  9;
            9,  9,  9,  9,  9,  9,  9);
        let (x, y) = compute_centroid(&image_7x7, &bounding_box, 1.0);
        assert_abs_diff_eq!(x, 3.0, epsilon = 0.01);
        assert_abs_diff_eq!(y, 3.0, epsilon = 0.01);

        // Shift flux toward +x: centroid moves right of center, by less than
        // a full pixel.
        let image_7x7 = gray_image!(
            9,  9,  9,  9,  9,  9,  9;
            9, 10, 10, 10, 10, 10,  9;
            9, 10, 20, 40, 30, 10,  9;
            9, 10, 40, 80, 60, 10,  9;
            9, 10, 20, 40, 30, 10,  9;
            9, 10, 10, 10, 10, 10,  9;
            9,  9,  9,  9,  9,  9,  9);
        let (x, y) = compute_centroid(&image_7x7, &bounding_box, 1.0);
        assert!(x > 3.0 && x < 4.0, "expected centroid right of center, got {}", x);
        assert_abs_diff_eq!(y, 3.0, epsilon = 0.01);

        // Uniform region: nothing exceeds the threshold, so we fall back to
        // the box center rather than dividing by zero.
        let flat_7x7 = gray_image!(
            9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9);
        let (x, y) = compute_centroid(&flat_7x7, &bounding_box, 1.0);
        assert_abs_diff_eq!(x, 3.0, epsilon = 0.01);
        assert_abs_diff_eq!(y, 3.0, epsilon = 0.01);
    }

    #[test]
    fn test_classify_pixel_min_neighbor_excess() {
        let sigma_noise_2 = 10;
        let floor = min_neighbor_excess_8(1.0);

        // A marginal candidate: the center is only 5 above a background of 10,
        // just clearing the brightness gate. Its neighbors carry a little flux
        // -- more than half the center's excess, but not significantly higher
        // than background.
        let above =  [10, 10, 10, 11, 10, 10, 10];
        let center = [10, 10, 11, 15, 11, 10, 10];
        let below =  [10, 10, 10, 10, 10, 10, 10];
        // Judged by the ratio alone, the neighbors look like a star.
        assert_eq!(classify_pixel(&above, &center, &below, sigma_noise_2, 0).0,
                   PixelHotType::Bright);
        // The floor requires neighbor flux significantly higher than
        // background, rather than merely a fraction of a faint center, so this
        // is rejected as hot.
        assert_eq!(classify_pixel(&above, &center, &below, sigma_noise_2, floor).0,
                   PixelHotType::Hot);

        // A bright candidate is unaffected: half its excess already exceeds the
        // floor, leaving the ratio test in control.
        let above_bright =  [10, 10, 10, 40, 10, 10, 10];
        let center_bright = [10, 10, 40, 80, 40, 10, 10];
        let below_bright =  [10, 10, 10, 40, 10, 10, 10];
        assert_eq!(classify_pixel(&above_bright, &center_bright, &below_bright,
                                  sigma_noise_2, floor).0,
                   PixelHotType::Bright);
        // As is a bright pixel with no neighbor support at all.
        let above_hot =  [10, 10, 10, 10, 10, 10, 10];
        let center_hot = [10, 10, 10, 80, 10, 10, 10];
        let below_hot =  [10, 10, 10, 10, 10, 10, 10];
        assert_eq!(classify_pixel(&above_hot, &center_hot, &below_hot,
                                  sigma_noise_2, floor).0,
                   PixelHotType::Hot);
    }

    #[test]
    fn test_brightness() {
        let image_7x7 = gray_image!(
        9,  9,  9,   9,  9,  9,  9;
        9, 10, 10,  10, 10, 10,  9;
        9, 10, 12, 255, 12, 10,  9;
        9, 10, 14, 255, 14, 10,  9;
        9, 10, 12,  14, 30, 10,  9;
        9, 10, 10,  10, 10, 10,  9;
        9,  9,  9,   9,  9,  9,  9);
        let neighbors = Rect::at(1, 1).of_size(5, 5);
        let (brightness, num_sat, peak_value) = compute_brightness(&image_7x7, &neighbors);
        assert_abs_diff_eq!(brightness,
                            528.0, epsilon = 0.1);
        assert_eq!(num_sat, 2);
        assert_eq!(peak_value, 255);
    }

    #[test]
    #[should_panic]
    fn test_summarize_region_of_interest_left_edge() {
        let roi = Rect::at(2, 3).of_size(3, 2);
        let image_9x9 = gray_image!(
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9);
        // Cannot give ROI too close to left edges.
        let _roi_summary = summarize_region_of_interest(
            &image_9x9, &roi, 1.0, 5.0);
    }

    #[test]
    #[should_panic]
    fn test_summarize_region_of_interest_right_edge() {
        let roi = Rect::at(4, 3).of_size(3, 2);
        let image_9x9 = gray_image!(
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9);
        // Cannot give ROI too close to right edges.
        let _roi_summary = summarize_region_of_interest(
            &image_9x9, &roi, 1.0, 5.0);
    }

    #[test]
    fn test_summarize_region_of_interest_good_edges() {
        let roi = Rect::at(3, 3).of_size(3, 2);
        // 80 is a hot pixel: bright, but isolated in both axes, so it is
        // rejected as a peak candidate. The star peaking at 32 has extent in
        // both axes, as real stars do, so it wins the peak despite being
        // dimmer.
        let image_9x9 = gray_image!(
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  14, 9,  9,  9;
            9,  9,  9,  7,  80, 22, 10, 9,  9;
            9,  9,  9,  11, 20, 32, 14, 9,  9;
            9,  9,  9,  9,  12, 22, 10, 9,  9;
            9,  9,  9,  9,  9,  14, 9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9);
        // ROI is correct distance from left+right edges.
        let roi_summary = summarize_region_of_interest(
            &image_9x9, &roi, 1.0, 5.0);
        assert_eq!(roi_summary.histogram[7], 1);
        assert_eq!(roi_summary.histogram[11], 1);
        assert_eq!(roi_summary.histogram[20], 1);
        assert_eq!(roi_summary.histogram[22], 1);
        assert_eq!(roi_summary.histogram[32], 1);
        // The histogram holds raw values, so the hot pixel appears at its own
        // 80 rather than as an interpolated neighbor average. The histogram
        // is consulted only for percentiles over a large ROI, where the few
        // hot pixels cannot move the result.
        assert_eq!(roi_summary.histogram[80], 1);
        assert_eq!(roi_summary.histogram[14], 0);
        // The hot pixel is not the peak, but it can influence the centroided
        // position of the peak and the peak_value. It is still present
        // (unreplaced) in the image the centroid is computed over, pulling the
        // result up and to the left of the true peak at (5, 4).
        assert_abs_diff_eq!(roi_summary.peak_x,
                            4.92, epsilon = 0.1);
        assert_abs_diff_eq!(roi_summary.peak_y,
                            4.00, epsilon = 0.1);
        assert_abs_diff_eq!(roi_summary.peak_value,
                            24.67, epsilon = 0.1);
    }

    #[test]
    fn test_summarize_region_of_interest_peak_upper_left() {
        let roi = Rect::at(3, 3).of_size(3, 2);
        let image_9x9 = gray_image!(
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  99, 99, 69, 9,  9,  9,  9,  9;
            9,  99, 80, 60, 50,  9,  9,  9,  9;
            9,  69, 60, 50, 40, 10, 9,  9,  9;
            9,  9,  50, 40, 20, 10, 9, 9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9);
        let roi_summary = summarize_region_of_interest(
            &image_9x9, &roi, 1.0, 5.0);
        assert_abs_diff_eq!(roi_summary.peak_x,
                            3.0, epsilon = 0.01);
        assert_abs_diff_eq!(roi_summary.peak_y,
                            3.0, epsilon = 0.01);
    }

    #[test]
    fn test_summarize_region_of_interest_peak_lower_right() {
        let roi = Rect::at(3, 3).of_size(3, 2);
        let image_9x9 = gray_image!(
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9, 20, 40, 50,  9,  9;
            9,  9,  9,  9, 40, 50, 60,  9,  9;
            9,  9,  9,  9, 50, 60, 80,  9,  9;
            9,  9,  9,  9, 59, 69, 89,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9);
        let roi_summary = summarize_region_of_interest(
            &image_9x9, &roi, 1.0, 5.0);
        assert_abs_diff_eq!(roi_summary.peak_x,
                            5.0, epsilon = 0.01);
        assert_abs_diff_eq!(roi_summary.peak_y,
                            4.0, epsilon = 0.01);
    }

    #[test]
    fn test_summarize_region_of_interest_peak_tiebreak_to_center() {
        let roi = Rect::at(3, 3).of_size(5, 5);
        let image_11x11 = gray_image!(
            9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9, 11,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9, 11,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9, 11,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9;
            9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9);
        let roi_summary = summarize_region_of_interest(
            &image_11x11, &roi, 1.0, 5.0);
        assert_abs_diff_eq!(roi_summary.peak_x,
                            5.5, epsilon = 0.01);
        assert_abs_diff_eq!(roi_summary.peak_y,
                            5.5, epsilon = 0.01);
    }

    // Builds a synthetic night-sky frame: noisy dark background, tight
    // gaussian stars, and isolated single-pixel hot spots. Deterministic
    // (fixed seeds and a simple LCG), so results are reproducible run to run.
    //
    // Returns the image plus the positions of the brightest star and the
    // brightest hot pixel, so callers can assert which one wins the peak.
    fn synth_star_field(width: u32, height: u32)
                        -> (GrayImage, (i32, i32), (i32, i32)) {
        // Background: mean 10, stddev 2, as a dark sky frame.
        let mut image = gaussian_noise(&GrayImage::new(width, height),
                                       10.0, 2.0, 42);

        // Simple deterministic LCG; we want reproducibility, not quality.
        let mut rng_state = 0x2545F491_4F6CDD1D_u64;
        let mut next = |bound: u32| -> u32 {
            rng_state = rng_state
                .wrapping_mul(6364136223846793005)
                .wrapping_add(1442695040888963407);
            ((rng_state >> 33) as u32) % bound
        };

        // Keep everything well inside the edges: summarize_region_of_interest
        // requires margin around the ROI, and stars need room for their tails.
        let margin = 16_i32;
        let x_range = (width as i32 - 2 * margin) as u32;
        let y_range = (height as i32 - 2 * margin) as u32;

        // Tight gaussian stars. sigma below one pixel, as real stars are on a
        // well-focused sensor, so most of the flux lands in a 3x3.
        let sigma = 0.9_f64;
        let num_stars = 400;
        let mut brightest_star = (0_i32, 0_i32);
        let mut brightest_star_amp = 0_u32;
        for i in 0..num_stars {
            let cx = margin + next(x_range) as i32;
            let cy = margin + next(y_range) as i32;
            // Amplitudes spread over a realistic range; the brightest star
            // stays below the hot pixels so we can tell them apart.
            let amp = 40 + next(140);
            if amp > brightest_star_amp {
                brightest_star_amp = amp;
                brightest_star = (cx, cy);
            }
            for dy in -3..=3_i32 {
                for dx in -3..=3_i32 {
                    let r2 = (dx * dx + dy * dy) as f64;
                    let falloff = (-r2 / (2.0 * sigma * sigma)).exp();
                    let add = (amp as f64 * falloff) as u32;
                    if add == 0 {
                        continue;
                    }
                    let px = (cx + dx) as u32;
                    let py = (cy + dy) as u32;
                    let cur = image.get_pixel(px, py).0[0] as u32;
                    image.put_pixel(px, py,
                                    Luma([cur.saturating_add(add).min(255) as u8]));
                }
            }
            // Silence the unused-variable warning on the loop counter without
            // changing the loop shape.
            let _ = i;
        }

        // Isolated single-pixel hot spots, some at full scale. These are what
        // the hot pixel logic must reject as peak candidates.
        let num_hot = 60;
        let mut brightest_hot = (0_i32, 0_i32);
        let mut brightest_hot_amp = 0_u32;
        for _ in 0..num_hot {
            let hx = margin + next(x_range) as i32;
            let hy = margin + next(y_range) as i32;
            // Well above the brightest star, up to saturation.
            let amp = 200 + next(56);
            if amp > brightest_hot_amp {
                brightest_hot_amp = amp;
                brightest_hot = (hx, hy);
            }
            image.put_pixel(hx as u32, hy as u32, Luma([amp.min(255) as u8]));
        }

        (image, brightest_star, brightest_hot)
    }

    // A full-size frame exercising summarize_region_of_interest over a
    // realistic ROI (the full-height centered square that detect_engine uses
    // in focus mode). Large enough to time meaningfully; reports per-call
    // timing so changes to the function can be compared:
    //
    //   cargo test --release test_summarize_region_of_interest_large -- --nocapture
    //
    // Build in release: a debug build is dominated by bounds checks and is
    // not indicative.
    #[test]
    fn test_summarize_region_of_interest_large() {
        let (width, height) = (1920_u32, 1080_u32);
        let (image, _brightest_star, _brightest_hot) =
            synth_star_field(width, height);

        // The centered square crop, inset to leave the margin that
        // summarize_region_of_interest requires around the ROI.
        let inset = 8_i32;
        let square = height as i32;
        let roi = Rect::at((width as i32 - square) / 2 + inset, inset)
            .of_size((square - 2 * inset) as u32,
                     (square - 2 * inset) as u32);

        let noise_estimate = estimate_noise_from_image(&image);

        // One warm-up call (page faults, cache) that we don't time.
        let roi_summary =
            summarize_region_of_interest(&image, &roi, noise_estimate, 8.0);

        let iterations = 10;
        let mut best = Duration::MAX;
        let mut total_elapsed = Duration::ZERO;
        for _ in 0..iterations {
            let start = Instant::now();
            let s = summarize_region_of_interest(
                &image, &roi, noise_estimate, 8.0);
            let elapsed = start.elapsed();
            total_elapsed += elapsed;
            best = best.min(elapsed);
            // Keep the call from being optimized away.
            assert_eq!(s.histogram.len(), 256);
        }
        let pixels = roi.width() as f64 * roi.height() as f64;
        println!(
            "summarize_region_of_interest over {}x{} ROI ({:.2} Mpixel): \
             best {:?}, mean {:?} ({:.1} Mpixel/s at best)",
            roi.width(), roi.height(), pixels / 1e6,
            best, total_elapsed / iterations,
            pixels / best.as_secs_f64() / 1e6);

        // Every ROI pixel is histogrammed exactly once.
        let total: u32 = roi_summary.histogram.iter().sum();
        assert_eq!(total as i32, roi.width() as i32 * roi.height() as i32);

        // The background dominates, so the low end of the histogram holds the
        // bulk of the pixels while the bright tail stays sparse.
        let background: u32 = roi_summary.histogram[..32].iter().sum();
        assert!(background > total * 9 / 10,
                "expected background to dominate, got {} of {}",
                background, total);

        // The peak must land on a star, not on one of the much brighter hot
        // pixels: peak_value is a 3x3 mean around the peak, so an isolated
        // 255 spike surrounded by background would score far below a star.
        assert!(roi_summary.peak_value < 200.0,
                "peak_value {} suggests a hot pixel won the peak",
                roi_summary.peak_value);

        // The peak is inside the ROI.
        assert!(roi_summary.peak_x >= roi.left() as f64
                && roi_summary.peak_x <= roi.right() as f64,
                "peak_x {} outside ROI", roi_summary.peak_x);
        assert!(roi_summary.peak_y >= roi.top() as f64
                && roi_summary.peak_y <= roi.bottom() as f64,
                "peak_y {} outside ROI", roi_summary.peak_y);
    }

}  // mod tests.
