//! CedarDetect inside the calling process.
//!
//! The photo is decoded and turned upright once. Each extract reads a crop of
//! that gray image in memory, so a sliding frame costs only the detection.

#[path = "cedar/algorithm.rs"]
#[allow(dead_code, clippy::all)]
mod algorithm;
#[path = "cedar/histogram_funcs.rs"]
#[allow(dead_code, clippy::all)]
mod histogram_funcs;
#[path = "cedar/image_funcs.rs"]
#[allow(dead_code, clippy::all)]
mod image_funcs;

use std::ffi::{c_char, CStr};
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::time::Instant;

use image::{DynamicImage, GrayImage, ImageDecoder, ImageReader};

use algorithm::{estimate_noise_from_image, get_stars_from_image};

pub struct Photo {
    gray: GrayImage,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CedarStar {
    /// Crop pixels. (0.5, 0.5) is the middle of the top-left pixel.
    pub x: f64,
    pub y: f64,
    /// Aperture sum less the local median, on the full photo.
    pub flux: f64,
    /// Cedar's own background-subtracted sum.
    pub brightness: f64,
}

/// Decodes and turns the photo upright. Null when it cannot be read.
///
/// # Safety
/// `path` is a NUL-terminated UTF-8 string. `width` and `height` are writable.
#[no_mangle]
pub unsafe extern "C" fn cedar_open(
    path: *const c_char,
    width: *mut u32,
    height: *mut u32,
) -> *mut Photo {
    let opened = catch_unwind(AssertUnwindSafe(|| -> Option<Photo> {
        let path = CStr::from_ptr(path).to_str().ok()?;
        decode(ImageReader::open(path).ok()?)
    }));
    hand_out(opened, width, height)
}

/// The same as [cedar_open], from an encoded photo already in memory.
///
/// # Safety
/// `bytes` holds `length` readable bytes. `width` and `height` are writable.
#[no_mangle]
pub unsafe extern "C" fn cedar_open_memory(
    bytes: *const u8,
    length: usize,
    width: *mut u32,
    height: *mut u32,
) -> *mut Photo {
    if bytes.is_null() || length == 0 {
        return std::ptr::null_mut();
    }
    let data = std::slice::from_raw_parts(bytes, length);
    let opened = catch_unwind(AssertUnwindSafe(|| -> Option<Photo> {
        decode(ImageReader::new(std::io::Cursor::new(data)))
    }));
    hand_out(opened, width, height)
}

fn decode<R: std::io::BufRead + std::io::Seek>(reader: ImageReader<R>) -> Option<Photo> {
    let mut decoder = reader.with_guessed_format().ok()?.into_decoder().ok()?;
    let orientation = decoder.orientation().ok()?;
    let mut picture = DynamicImage::from_decoder(decoder).ok()?;
    picture.apply_orientation(orientation);
    Some(Photo { gray: luma_601(&picture) })
}

unsafe fn hand_out(
    opened: std::thread::Result<Option<Photo>>,
    width: *mut u32,
    height: *mut u32,
) -> *mut Photo {
    match opened {
        Ok(Some(photo)) => {
            *width = photo.gray.width();
            *height = photo.gray.height();
            Box::into_raw(Box::new(photo))
        }
        _ => std::ptr::null_mut(),
    }
}

/// # Safety
/// `photo` came from [cedar_open] and is not used afterwards.
#[no_mangle]
pub unsafe extern "C" fn cedar_close(photo: *mut Photo) {
    if !photo.is_null() {
        drop(Box::from_raw(photo));
    }
}

/// Stars in one crop, brightest aperture first. Returns the count written,
/// or -1 when the crop or the arguments are not usable.
///
/// # Safety
/// `photo` came from [cedar_open]. `out` holds `max_stars` entries. The other
/// pointers are writable.
#[no_mangle]
pub unsafe extern "C" fn cedar_extract(
    photo: *const Photo,
    x0: u32,
    y0: u32,
    width: u32,
    height: u32,
    sigma: f64,
    binning: u32,
    max_stars: u32,
    out: *mut CedarStar,
    found: *mut u32,
    noise: *mut f64,
    elapsed_ms: *mut f64,
) -> i32 {
    if photo.is_null() || out.is_null() || !matches!(binning, 1 | 2 | 4) {
        return -1;
    }
    let photo = &*photo;
    let full = &photo.gray;
    if width < 32
        || height < 32
        || x0.saturating_add(width) > full.width()
        || y0.saturating_add(height) > full.height()
    {
        return -1;
    }
    let started = Instant::now();
    let result = catch_unwind(AssertUnwindSafe(|| {
        let crop = crop_rows(full, x0, y0, width, height);
        let noise_estimate = estimate_noise_from_image(&crop);
        let (stars, _hot, _binned) =
            get_stars_from_image(&crop, None, noise_estimate, sigma, binning);
        let mut ranked: Vec<CedarStar> = stars
            .iter()
            .map(|star| CedarStar {
                x: star.centroid_x,
                y: star.centroid_y,
                flux: aperture_flux(
                    full,
                    star.centroid_x + x0 as f64,
                    star.centroid_y + y0 as f64,
                    8,
                ),
                brightness: star.brightness,
            })
            .collect();
        ranked.sort_by(|a, b| b.flux.total_cmp(&a.flux));
        (ranked, stars.len(), noise_estimate)
    }));
    let Ok((ranked, total, noise_estimate)) = result else {
        return -1;
    };
    let count = ranked.len().min(max_stars as usize);
    std::ptr::copy_nonoverlapping(ranked.as_ptr(), out, count);
    if !found.is_null() {
        *found = total as u32;
    }
    if !noise.is_null() {
        *noise = noise_estimate;
    }
    if !elapsed_ms.is_null() {
        *elapsed_ms = started.elapsed().as_secs_f64() * 1000.0;
    }
    count as i32
}

/// Eight-byte aligned memory for callers that have no allocator of their own.
#[no_mangle]
pub extern "C" fn cedar_alloc(bytes: usize) -> *mut u8 {
    match std::alloc::Layout::from_size_align(bytes.max(8), 8) {
        Ok(layout) => unsafe { std::alloc::alloc_zeroed(layout) },
        Err(_) => std::ptr::null_mut(),
    }
}

/// # Safety
/// `pointer` came from [cedar_alloc] with the same `bytes`.
#[no_mangle]
pub unsafe extern "C" fn cedar_free(pointer: *mut u8, bytes: usize) {
    if pointer.is_null() {
        return;
    }
    if let Ok(layout) = std::alloc::Layout::from_size_align(bytes.max(8), 8) {
        std::alloc::dealloc(pointer, layout);
    }
}

/// The same gray as PIL's "L" mode, which the Cedar sidecars were made with.
fn luma_601(picture: &DynamicImage) -> GrayImage {
    let rgb = picture.to_rgb8();
    let (width, height) = rgb.dimensions();
    let mut gray = Vec::with_capacity((width * height) as usize);
    for pixel in rgb.as_raw().chunks_exact(3) {
        let value = (pixel[0] as u32 * 19595 + pixel[1] as u32 * 38470 + pixel[2] as u32 * 7471
            + 0x8000)
            >> 16;
        gray.push(value as u8);
    }
    GrayImage::from_raw(width, height, gray).expect("gray buffer matches the photo")
}

fn crop_rows(full: &GrayImage, x0: u32, y0: u32, width: u32, height: u32) -> GrayImage {
    let stride = full.width() as usize;
    let raw = full.as_raw();
    let mut pixels = Vec::with_capacity((width * height) as usize);
    for row in y0 as usize..(y0 + height) as usize {
        let start = row * stride + x0 as usize;
        pixels.extend_from_slice(&raw[start..start + width as usize]);
    }
    GrayImage::from_raw(width, height, pixels).expect("crop buffer matches its size")
}

/// A street lamp or the church outshines Cedar's peak order, so stars are
/// ranked by the light inside a fixed aperture.
fn aperture_flux(full: &GrayImage, x: f64, y: f64, radius: i64) -> f64 {
    let width = full.width() as i64;
    let height = full.height() as i64;
    let cx = x.round() as i64;
    let cy = y.round() as i64;
    let x0 = (cx - radius).max(0);
    let x1 = (cx + radius + 1).min(width);
    let y0 = (cy - radius).max(0);
    let y1 = (cy + radius + 1).min(height);
    if x0 >= x1 || y0 >= y1 {
        return 0.0;
    }
    let raw = full.as_raw();
    let mut values = Vec::with_capacity(((x1 - x0) * (y1 - y0)) as usize);
    let mut sum = 0.0;
    for row in y0..y1 {
        let start = (row * width) as usize;
        for &value in &raw[start + x0 as usize..start + x1 as usize] {
            values.push(value);
            sum += value as f64;
        }
    }
    values.sort_unstable();
    let n = values.len();
    let median = if n % 2 == 1 {
        values[n / 2] as f64
    } else {
        (values[n / 2 - 1] as f64 + values[n / 2] as f64) / 2.0
    };
    sum - median * n as f64
}
