// Copyright (c) 2025 Steven Rosenthal smr@dt3.org
// See LICENSE file in root directory for license terms.

use image::GrayImage;
use imageproc::rect::Rect;
use std::sync::OnceLock;

pub type Bin2x2Fn = fn(&GrayImage) -> GrayImage;

// Scans one row of `image` for star candidates and appends (x, y) pairs to
// `candidates`. `rownum` is the row index. `sigma_noise_2` and `sigma_noise_3`
// are precomputed thresholds passed from scan_image_for_candidates().
pub type ScanRowFn = fn(image: &GrayImage,
                        rownum: usize,
                        sigma_noise_2: i16,
                        sigma_noise_3: i16,
                        candidates: &mut Vec<(i32, i32)>);

static BIN2X2_FN: OnceLock<Bin2x2Fn> = OnceLock::new();
static SCAN_ROW_FN: OnceLock<ScanRowFn> = OnceLock::new();

pub fn set_binner(bin2x2: Bin2x2Fn) {
    log::info!("Setting image binning function.");
    let _ = BIN2X2_FN.set(bin2x2); // Ignores error if already set.
}

pub fn set_row_scanner(scan_row: ScanRowFn) {
    log::info!("Setting row scanner function.");
    let _ = SCAN_ROW_FN.set(scan_row); // Ignores error if already set.
}

pub fn get_row_scanner() -> Option<ScanRowFn> {
    SCAN_ROW_FN.get().copied()
}

pub fn bin_2x2(image: &GrayImage) -> GrayImage {
    match BIN2X2_FN.get() {
        Some(f) => f(image),
        None => bin_2x2_default(image),
    }
}

/// Returns a histogram of pixel values from the given region of `image`.
/// Panics if the region extends outside the image bounds.
pub fn histogram_from_region(image: &GrayImage, region: &Rect) -> [u32; 256] {
    assert!(region.left() >= 0 && region.top() >= 0,
            "region left/top must be non-negative");
    assert!(region.right() < image.width() as i32,
            "region right ({}) exceeds image width ({})",
            region.right(), image.width());
    assert!(region.bottom() < image.height() as i32,
            "region bottom ({}) exceeds image height ({})",
            region.bottom(), image.height());
    let width = image.width() as usize;
    let x0 = region.left() as usize;
    let x1 = region.right() as usize + 1;  // right() is inclusive
    let y0 = region.top() as u32;
    let y1 = region.bottom() as u32 + 1;   // bottom() is inclusive
    let pixels = image.as_raw();
    let mut histogram = [0u32; 256];
    for y in y0..y1 {
        let row_start = y as usize * width + x0;
        let row_end = y as usize * width + x1;
        for &p in &pixels[row_start..row_end] {
            histogram[p as usize] += 1;
        }
    }
    histogram
}

// Default implementation, used if set_binner() was not called.
fn bin_2x2_default(image: &GrayImage) -> GrayImage {
    let (width, height) = image.dimensions();
    let new_width = width / 2;
    let new_height = height / 2;
    let mut resized_image = Vec::with_capacity((new_width * new_height) as usize);
    let source_pixels = image.as_raw();
    for y in (0..height & !1).step_by(2) {
        for x in (0..width & !1).step_by(2) {
            let p1 = source_pixels[(y * width + x) as usize] as u16;
            let p2 = source_pixels[(y * width + x + 1) as usize] as u16;
            let p3 = source_pixels[((y + 1) * width + x) as usize] as u16;
            let p4 = source_pixels[((y + 1) * width + x + 1) as usize] as u16;
            resized_image.push(((p1 + p2 + p3 + p4) / 4) as u8);
        }
    }
    GrayImage::from_raw(new_width, new_height, resized_image).unwrap()
}

#[cfg(test)]
mod tests {
    use super::*;
    use image::{GrayImage, Luma};

    #[test]
    fn test_bin_2x2() {
        let mut img = GrayImage::new(4, 4);
        for y in 0..4 {
            for x in 0..4 {
                img.put_pixel(x, y, Luma([((y * 4 + x) as u8 + 1)]));
            }
        }

        let result = bin_2x2(&img);
        assert_eq!(result.dimensions(), (2, 2));
        assert_eq!(result.get_pixel(0, 0)[0], 3);   // [1,2,5,6]    -> 3
        assert_eq!(result.get_pixel(1, 0)[0], 5);   // [3,4,7,8]    -> 5
        assert_eq!(result.get_pixel(0, 1)[0], 11);  // [9,10,13,14] -> 11
        assert_eq!(result.get_pixel(1, 1)[0], 13);  // [11,12,15,16] -> 13
    }

    #[test]
    fn test_histogram_from_region() {
        // 4x4 image with pixel value == x + y*4 + 1 (values 1..=16).
        let mut img = GrayImage::new(4, 4);
        for y in 0..4u32 {
            for x in 0..4u32 {
                img.put_pixel(x, y, Luma([(x + y * 4 + 1) as u8]));
            }
        }

        // Full image: all 16 pixels, each value appears once.
        let full = Rect::at(0, 0).of_size(4, 4);
        let h = histogram_from_region(&img, &full);
        assert_eq!(h[..17].iter().sum::<u32>(), 16);
        for v in 1u8..=16 {
            assert_eq!(h[v as usize], 1, "value {v}");
        }

        // Center 2x2 (x=1..=2, y=1..=2): values 6,7,10,11.
        let center = Rect::at(1, 1).of_size(2, 2);
        let h = histogram_from_region(&img, &center);
        assert_eq!(h[6], 1);
        assert_eq!(h[7], 1);
        assert_eq!(h[10], 1);
        assert_eq!(h[11], 1);
        assert_eq!(h.iter().sum::<u32>(), 4);
    }

    #[test]
    #[should_panic(expected = "region right")]
    fn test_histogram_from_region_oob_right() {
        let img = GrayImage::new(4, 4);
        histogram_from_region(&img, &Rect::at(0, 0).of_size(5, 4));
    }

    #[test]
    #[should_panic(expected = "region bottom")]
    fn test_histogram_from_region_oob_bottom() {
        let img = GrayImage::new(4, 4);
        histogram_from_region(&img, &Rect::at(0, 0).of_size(4, 5));
    }
}
