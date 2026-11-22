// Copyright (c) 2026 Steven Rosenthal smr@dt3.org
// See LICENSE file in root directory for license terms.

use std::cmp::{max, min};

#[derive(Debug)]
pub struct HistogramStats {
    pub mean: f64,
    pub median: usize,
    pub stddev: f64,
}

pub fn stats_for_histogram(histogram: &[u32]) -> HistogramStats {
    let mut count = 0;
    let mut first_moment = 0;
    for (h, &bin_count) in histogram.iter().enumerate() {
        count += bin_count;
        first_moment += bin_count * h as u32;
    }
    if count == 0 {
        return HistogramStats{mean: 0.0, median: 0, stddev: 0.0};
    }
    let mean = first_moment as f64 / count as f64;
    let mut second_moment: f64 = 0.0;
    let mut sub_count = 0;
    let mut median = 0;
    for (h, &bin_count) in histogram.iter().enumerate() {
        second_moment += bin_count as f64 * (h as f64 - mean) * (h as f64 - mean);
        if sub_count < count / 2 {
            sub_count += bin_count;
            if sub_count >= count / 2 {
                median = h;
            }
        }
    }
    let stddev = (second_moment / count as f64).sqrt();
    HistogramStats{mean, median, stddev}
}

/// Given a histogram of u8 data, and the number of points, return an estimate
/// of the data's dark level (u8 range, as f32).
pub fn estimate_dark_level(pixel_histogram: &[u32], npoints: usize) -> f32 {
    // Method: take the mean value of the bottom 1% of points.
    let one_percent = (npoints / 100) as u32;
    if one_percent == 0 {
        // Too little data; just return the lowest non-zero bin.
        for (h, &count) in pixel_histogram.iter().enumerate() {
            if count > 0 {
                return h as f32;
            }
        }
    }

    // `accum` is first moment of first one_percent of points.
    let mut accum = 0;
    let mut accum_remaining = one_percent;

    for (h, &bin_count) in pixel_histogram.iter().enumerate() {
        if bin_count == 0 {
            continue;
        }
        if bin_count < accum_remaining {
            accum += h as u32 * bin_count;
            accum_remaining -= bin_count;
            continue;
        }
        accum += h as u32 * accum_remaining;
        break;
    }

    // Compute mean from first moment.
    accum as f32 / one_percent as f32
}

/// Return the histogram bin number N such that the total number of bin entries
/// at or below N exceeds `fraction` * the total number of bin entries over the
/// entire histogram.
pub fn get_level_for_fraction(histogram: &[u32], fraction: f64) -> usize {
    assert!(fraction >= 0.0);
    assert!(fraction <= 1.0);
    let total_count: u32 = histogram.iter().sum();
    let goal = (fraction * total_count as f64) as u32;
    let mut count = 0;
    for (h, &bin_count) in histogram.iter().enumerate() {
        count += bin_count;
        if count >= goal {
            return h;
        }
    }
    unreachable!()  // Should not get here.
}

/// Return the average of the N highest histogram entry values.
pub fn average_top_values(histogram: &[u32], num_top_values: usize) -> u8 {
    let mut accum_count = 0;
    let mut accum_val: u32 = 0;
    for bin in (1..256).rev() {
        let remain = num_top_values - accum_count;
        if remain == 0 {
            break;
        }
        let count = min(histogram[bin], remain as u32);
        accum_val += bin as u32 * count;
        accum_count += count as usize;
    }
    if accum_count == 0 {
        0
    } else {
        max(accum_val / accum_count as u32, 1) as u8
    }
}

/// Updates `histogram`, removing counts deemed to be contributed by stars. We
/// assume that the histogram was obtained from an image consisting of well
/// focused stars on a mostly dark background.
/// We form an estimate of the background noise and use this to remove counts
/// from histogram bins of pixel values greater than `sigma` times the noise
/// estimate.
/// What remains is deemed to be the histogram of the non-star image pixels.
pub fn remove_stars_from_histogram(histogram: &mut [u32], sigma: f64) {
    let pixel_count: u32 = histogram.iter().sum();
    // Do a sloppy trim of the brightest pixels; this will give us a (probably
    // overly) de-starred mean and stddev that we can use for a more precise
    // trim.
    let mut copied_histogram = histogram.to_vec();
    trim_histogram(&mut copied_histogram, pixel_count * 9 / 10);
    let stats = stats_for_histogram(&copied_histogram);
    // Any pixel whose value is sigma * stddev above the mean is deemed a star and
    // kicked out of the histogram.
    let star_cutoff = (stats.mean +
                       sigma * f64::max(stats.stddev, 1.0)) as usize;
    for (h, bin) in histogram.iter_mut().enumerate() {
        if h >= star_cutoff {
            *bin = 0;
        }
    }
}

fn trim_histogram(histogram: &mut [u32], count_to_keep: u32) {
    let mut count = 0;
    for bin in histogram.iter_mut() {
        let bin_count = *bin;
        if count + bin_count > count_to_keep {
            let excess = count + bin_count - count_to_keep;
            *bin -= excess;
        }
        count += *bin;
    }
}


#[cfg(test)]
mod tests {
    use crate::histogram_funcs::{estimate_dark_level, stats_for_histogram};

    #[test]
    fn test_stats_for_histogram() {
        let mut histogram = [0_u32; 1024];
        histogram[10] = 2;
        histogram[20] = 2;
        let stats = stats_for_histogram(&histogram);
        assert_eq!(stats.mean, 15.0);
        assert_eq!(stats.median, 10);
        assert_eq!(stats.stddev, 5.0);
    }

    #[test]
    fn test_estimate_dark_level_small_width() {
        let mut row_histogram = [0_u32; 256];

        // Width is small, so 1% count underflows.
        row_histogram[2] = 1;
        row_histogram[3] = 9;
        assert_eq!(estimate_dark_level(&row_histogram, 10), 2.0);
    }

    #[test]
    fn test_estimate_dark_level() {
        let mut row_histogram = [0_u32; 256];

        // 1% is 10 values, first 5 from bin 2 and second 5 from bin 4.
        row_histogram[2] = 5;
        row_histogram[4] = 15;
        row_histogram[10] = 980;
        assert_eq!(estimate_dark_level(&row_histogram, 1000), 3.0);
    }
}  // mod tests.
