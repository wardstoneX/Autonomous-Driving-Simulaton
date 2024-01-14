# Changelog

All notable changes by HENSOLDT Cyber GmbH to this 3rd party module included in
the TRENTOS SDK will be documented in this file.

For more details it is recommended to compare the 3rd party module at hand with
the previous versions of the TRENTOS SDK or the baseline version.

## [1.0]

### Changed

- Adapt module for the Raspberry Pi 3 Model B+ platform.
- Extract GPIO and SPI related features into `bcm2837_gpio` and `bcm2837_spio`
and remove unused functionalities.
- Apply our coding style / formatting.

### Added

- Start development of bcm2837 based on `bcm2835-1.68.tar.gz` from
<https://www.airspayce.com/mikem/bcm2835/>.
