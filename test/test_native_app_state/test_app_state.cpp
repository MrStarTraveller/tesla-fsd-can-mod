#define LEGACY
#include <unity.h>
#include "app.h"

void setUp() {}
void tearDown() {}

void test_app_state_reports_starting_when_not_ready_without_fault() {
    CanDriverDiagnostics previous = {};
    CanDriverDiagnostics current = {};

    auto state = app_detail::nextRuntimeState(false, false, previous, current, false);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(app_detail::AppRuntimeState::Starting),
        static_cast<uint8_t>(state)
    );
}

void test_app_state_reports_init_fault_when_init_failed() {
    CanDriverDiagnostics previous = {};
    CanDriverDiagnostics current = {};

    auto state = app_detail::nextRuntimeState(false, true, previous, current, false);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(app_detail::AppRuntimeState::InitFault),
        static_cast<uint8_t>(state)
    );
}

void test_app_state_reports_processing_when_frame_was_handled() {
    CanDriverDiagnostics previous = {};
    CanDriverDiagnostics current = {};

    auto state = app_detail::nextRuntimeState(true, false, previous, current, true);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(app_detail::AppRuntimeState::Processing),
        static_cast<uint8_t>(state)
    );
}

void test_app_state_reports_idle_when_only_no_message_counter_changes() {
    CanDriverDiagnostics previous = {};
    previous.noMessageReads = 10;
    previous.lastError = CanDriverError::NoMessage;
    CanDriverDiagnostics current = previous;
    current.noMessageReads = 11;

    auto state = app_detail::nextRuntimeState(true, false, previous, current, false);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(app_detail::AppRuntimeState::Idle),
        static_cast<uint8_t>(state)
    );
}

void test_app_state_reports_runtime_fault_on_new_send_failure() {
    CanDriverDiagnostics previous = {};
    CanDriverDiagnostics current = {};
    current.sendFailures = 1;
    current.lastError = CanDriverError::SendFailed;

    auto state = app_detail::nextRuntimeState(true, false, previous, current, false);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(app_detail::AppRuntimeState::RuntimeFault),
        static_cast<uint8_t>(state)
    );
}

void test_app_state_reports_runtime_fault_on_recovery_event() {
    CanDriverDiagnostics previous = {};
    CanDriverDiagnostics current = {};
    current.recoveries = 1;
    current.lastError = CanDriverError::BusOff;

    auto state = app_detail::nextRuntimeState(true, false, previous, current, false);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(app_detail::AppRuntimeState::RuntimeFault),
        static_cast<uint8_t>(state)
    );
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_app_state_reports_starting_when_not_ready_without_fault);
    RUN_TEST(test_app_state_reports_init_fault_when_init_failed);
    RUN_TEST(test_app_state_reports_processing_when_frame_was_handled);
    RUN_TEST(test_app_state_reports_idle_when_only_no_message_counter_changes);
    RUN_TEST(test_app_state_reports_runtime_fault_on_new_send_failure);
    RUN_TEST(test_app_state_reports_runtime_fault_on_recovery_event);

    return UNITY_END();
}
