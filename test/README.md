# Test Directory

Place your unit tests here.

## Running Tests

```bash
pio test
```

## Example Test

```cpp
// test_example.cpp
#include <Arduino.h>
#include <unity.h>

void test_led_builtin_pin_number(void) {
    TEST_ASSERT_EQUAL(2, LED_BUILTIN);
}

void test_addition(void) {
    TEST_ASSERT_EQUAL(4, 2 + 2);
}

void setup() {
    delay(2000); // Wait for serial
    UNITY_BEGIN();
    RUN_TEST(test_led_builtin_pin_number);
    RUN_TEST(test_addition);
    UNITY_END();
}

void loop() {
    // Nothing here
}
```

## Documentation

https://docs.platformio.org/en/latest/advanced/unit-testing/index.html
