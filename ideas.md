# Ideas for future work on the pixlAnalyzer

radio chip: nrf52832, no PMIC.

The radio covers 2.4GHz ISM band, which is commonly used for various wireless communication protocols such as Bluetooth, Wi-Fi, and Zigbee.

Right now it only measures RSSI for spectrum analysis, on the range of 2400-2487MHz, with 1MHz steps.
(128 pixel display, so on average 1,45 pixels per frequency, some frequencies will have 2 pixels, some 1 pixel)

This functionality can be expanded in several ways:

1. **Frequency range visualisation**: Give visual clues of the type of bands that exist.  
2. **Signal identification**: Implement decoding for common protocols like Bluetooth Low Energy (BLE), IEEE 802.15.4 (Zigbee, Thread,..), or Wi-Fi. This would allow the device to not only detect signals but also identify the data being transmitted.

Some shortcomings that can be improved (and have not been done yet):

1. better voltage calibration. On my OLED based device: 2.4V becomes 2.5V on screen, and 4V becomes 4.15V on screen.
2. allow removing the speed indicator top left, it is not very useful outside of dev use.

And why not move to the latest SDK version while at it?

## Frequency range visualisation

The 2.4GHz ISM band is divided into several channels used by different protocols. Here are some common channel allocations:

| Center Frequency<br>(MHz) | IEEE 802.15.4<br>channel, 5MHz wide | 802.11b/g/n WiFi<br>channel, 22/20/40MHz wide | BLE<br>channel, 2MHz wide |
|------|----|----|----|
| 2400 |    |    |    |
| 2401 |    |    |    |
| 2402 |    |    | 37 |
| 2403 |    |    |    |
| 2404 |    |    | 0  |
| 2405 | 11 |    |    |
| 2406 |    |    | 1  |
| 2407 |    |    |    |
| 2408 |    |    | 2  |
| 2409 |    |    |    |
| 2410 | 12 |    | 3  |
| 2411 |    |    |    |
| 2412 |    |  1 | 4  |
| 2413 |    |    |    |
| 2414 |    |    | 5  |
| 2415 | 13 |    |    |
| 2416 |    |    | 6  |
| 2417 |    |  2 |    |
| 2418 |    |    | 7  |
| 2419 |    |    |    |
| 2420 | 14 |    | 8  |
| 2421 |    |    |    |
| 2422 |    |  3 | 9  |
| 2423 |    |    |    |
| 2424 |    |    | 10 |
| 2425 | 15 |    |    |
| 2426 |    |    | 38 |
| 2427 |    | (4)|    |
| 2428 |    |    | 11 |
| 2429 |    |    |    |
| 2430 | 16 |    | 12 |
| 2431 |    |    |    |
| 2432 |    |  5 | 13 |
| 2433 |    |    |    |
| 2434 |    |    | 14 |
| 2435 | 17 |    |    |
| 2436 |    |    | 15 |
| 2437 |    |  6 |    |
| 2438 |    |    | 16 |
| 2439 |    |    |    |
| 2440 | 18 |    | 17 |
| 2441 |    |    |    |
| 2442 |    |  7 | 18 |
| 2443 |    |    |    |
| 2444 |    |    | 19 |
| 2445 | 19 |    |    |
| 2446 |    |    | 20 |
| 2447 |    |  8 |    |
| 2448 |    |    | 21 |
| 2449 |    |    |    |
| 2450 | 20 |    | 22 |
| 2451 |    |    |    |
| 2452 |    |  9 | 23 |
| 2453 |    |    |    |
| 2454 |    |    | 24 |
| 2455 | 21 |    |    |
| 2456 |    |    | 25 |
| 2457 |    |(10)|    |
| 2458 |    |    | 26 |
| 2459 |    |    |    |
| 2460 | 22 |    | 27 |
| 2461 |    |    |    |
| 2462 |    | 11 | 28 |
| 2463 |    |    |    |
| 2464 |    |    | 29 |
| 2465 | 23 |    |    |
| 2466 |    |    | 30 |
| 2467 |    | 12 |    |
| 2468 |    |    | 31 |
| 2469 |    |    |    |
| 2470 | 24 |    | 32 |
| 2471 |    |    |    |
| 2472 |    | 13 | 33 |
| 2473 |    |    |    |
| 2474 |    |    | 34 |
| 2475 | 25 |    |    |
| 2476 |    |    | 35 |
| 2477 |    |    |    |
| 2478 |    |    | 36 |
| 2479 |    |    |    |
| 2480 | 26 |    | 39 |
| 2481 |    |    |    |
| 2482 |    |    |    |
| 2483 |    |    |    |
| 2484 |    | 14 |    |
| 2485 |    |    |    |
| 2486 |    |    |    |
| 2487 |    |    |    |

![ISM band channels](ism_band_channels.png)


## SDK links for the nRF52832

[nRF52832 Product Specification v1.9](https://docs.nordicsemi.com/bundle/nRF52832_PS_v1.9/resource/nRF52832_PS_v1.9.pdf)
[RADIO — 2.4 GHz Radio](https://docs.nordicsemi.com/bundle/ps_nrf52832/page/radio.html)
[SAADC — Successive approximation analog-to-digital converter](https://docs.nordicsemi.com/bundle/ps_nrf52832/page/saadc.html)