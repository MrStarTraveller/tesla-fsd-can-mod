#include <unity.h>
#include "can_frame_types.h"
#include "drivers/mock_driver.h"

static MockDriver driver;

void setUp() {
    driver.reset();
}

void tearDown() {}

void test_mock_driver_init_success_clears_error() {
    TEST_ASSERT_TRUE(driver.init());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CanDriverError::None), static_cast<uint8_t>(driver.diagnostics().lastError));
    TEST_ASSERT_EQUAL_UINT32(0, driver.diagnostics().initFailures);
}

void test_mock_driver_init_failure_updates_diagnostics() {
    driver.initResult = false;
    TEST_ASSERT_FALSE(driver.init());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CanDriverError::InitFailed), static_cast<uint8_t>(driver.diagnostics().lastError));
    TEST_ASSERT_EQUAL_UINT32(1, driver.diagnostics().initFailures);
}

void test_mock_driver_idle_read_marks_no_message() {
    CanFrame frame = {};
    TEST_ASSERT_FALSE(driver.read(frame));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CanDriverError::NoMessage), static_cast<uint8_t>(driver.diagnostics().lastError));
    TEST_ASSERT_EQUAL_UINT32(1, driver.diagnostics().noMessageReads);
}

void test_mock_driver_read_failure_updates_counter() {
    driver.idleReadError = CanDriverError::ReadFailed;
    CanFrame frame = {};
    TEST_ASSERT_FALSE(driver.read(frame));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CanDriverError::ReadFailed), static_cast<uint8_t>(driver.diagnostics().lastError));
    TEST_ASSERT_EQUAL_UINT32(1, driver.diagnostics().readFailures);
}

void test_mock_driver_successful_read_returns_frame() {
    CanFrame frame = {};
    CanFrame queued = {.id = 1021};
    queued.data[0] = 0x02;
    driver.enqueueRead(queued);

    TEST_ASSERT_TRUE(driver.read(frame));
    TEST_ASSERT_EQUAL_UINT32(1021, frame.id);
    TEST_ASSERT_EQUAL_UINT8(0x02, frame.data[0]);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CanDriverError::None), static_cast<uint8_t>(driver.diagnostics().lastError));
}

void test_mock_driver_send_failure_updates_counter() {
    CanFrame frame = {.id = 1006};
    driver.sendResult = false;
    driver.send(frame);

    TEST_ASSERT_EQUAL_UINT32(0, driver.sent.size());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CanDriverError::SendFailed), static_cast<uint8_t>(driver.diagnostics().lastError));
    TEST_ASSERT_EQUAL_UINT32(1, driver.diagnostics().sendFailures);
}

void test_mock_driver_successful_send_records_frame() {
    CanFrame frame = {.id = 69};
    driver.send(frame);

    TEST_ASSERT_EQUAL_UINT32(1, driver.sent.size());
    TEST_ASSERT_EQUAL_UINT32(69, driver.sent[0].id);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(CanDriverError::None), static_cast<uint8_t>(driver.diagnostics().lastError));
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_mock_driver_init_success_clears_error);
    RUN_TEST(test_mock_driver_init_failure_updates_diagnostics);
    RUN_TEST(test_mock_driver_idle_read_marks_no_message);
    RUN_TEST(test_mock_driver_read_failure_updates_counter);
    RUN_TEST(test_mock_driver_successful_read_returns_frame);
    RUN_TEST(test_mock_driver_send_failure_updates_counter);
    RUN_TEST(test_mock_driver_successful_send_records_frame);

    return UNITY_END();
}
