// tests/p2/test_p2.cpp
//
// YOUR test suite goes here. At least 12 assert-based test cases — see
// spec §5 for the required categories and the sample test for the
// expected level of rigor.
//
// This file is a stub so the project builds out of the box; replace the
// body of main() with your own tests.

#include "core/conversation.h"
#include "core/message.h"
#include "core/sentinel_scanner.h"
#include "harness/harness.h"
#include "model/replay_client.h"
#include "model/scripted_client.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// InputSource/OutputSink are abstract; main.cpp's implementations are
// private to that file, so these minimal fakes drive Harness::run() here.
class FakeInput : public InputSource {
public:
    explicit FakeInput(std::vector<std::string> lines) : lines_(std::move(lines)) {}
    std::string read_line() override {
        if (idx_ >= lines_.size()) { eof_ = true; return ""; }
        return lines_[idx_++];
    }
    bool is_eof() const override { return eof_; }
private:
    std::vector<std::string> lines_;
    std::size_t idx_ = 0;
    bool eof_ = false;
};

class CapturingOutput : public OutputSink {
public:
    void write(std::string_view text) override { captured_ += text; }
    const std::string& captured() const { return captured_; }
private:
    std::string captured_;
};



// ---------------------------------------------------------------------
// 1. Empty Conversation Bounds
// Confirms handling of empty conversations without out-of-bounds access.
// ---------------------------------------------------------------------
void test_empty_conversation_bounds() {
    Conversation c;
    assert(c.size() == 0);
    assert(c.begin() == c.end());

    bool threw = false;
    try { c.at(0); } catch (const std::out_of_range&) { threw = true; }
    assert(threw);

    threw = false;
    try { c.at(1000000); } catch (const std::out_of_range&) { threw = true; }
    assert(threw);

    std::cout << "test_empty_conversation_bounds passed\n";
}

// ---------------------------------------------------------------------
// 2. System Message Ordering
//  2 tests that confirm system messaging order is preserved
// test_system_message_ordering() handles Conversation
// test_harness_pins_system_message_first() handles harness
// ---------------------------------------------------------------------
void test_system_message_ordering() {
     // Conversation preserves append order regardless of role.
    Conversation c;
    c.append(Message(Role::System, "Be concise."));
    c.append(Message(Role::User, "hi"));
    c.append(Message(Role::Assistant, "hello"));

    assert(c.size() == 3);
    assert(c.at(0).role() == Role::System);
    assert(c.at(0).content() == "Be concise.");
    assert(c.at(1).role() == Role::User);
    assert(c.at(1).content() == "hi");
    assert(c.at(2).role() == Role::Assistant);
    assert(c.at(2).content() == "hello");

    std::cout << "test_system_message_ordering passed\n";
}

void test_harness_pins_system_message_first() {
    // Harness's constructor seeds the system message at index 0 before any turns.
    HarnessConfig cfg;
    cfg.system_message = "Be concise.";
    auto model = std::make_unique<ScriptedModelClient>("scripts/greeting.script");
    Harness harness(std::move(model), cfg);

    assert(harness.conversation().size() == 1);
    assert(harness.conversation().at(0).role() == Role::System);
    assert(harness.conversation().at(0).content() == "Be concise.");
    std::cout << "test_harness_pins_system_message_first passed\n";
}


// ---------------------------------------------------------------------
// 3. Rule of Five (Copy)
// Confirms that copy constructors allocate entirely different pointer addresses.
// ---------------------------------------------------------------------
void test_rule_of_five_copy() {
    Conversation a;
    a.append(Message(Role::User, "one"));
    a.append(Message(Role::User, "two"));

    Conversation b(a);  // copy constructor
    assert(b.begin() != a.begin());
    assert(b.size() == a.size());
    assert(b.at(0).content() == "one");
    assert(b.at(1).content() == "two");

    a.append(Message(Role::User, "three"));
    assert(b.size() == 2);  // b unaffected — independent buffers

    Conversation c;
    c = a;  // copy assignment
    assert(c.begin() != a.begin());
    assert(c.size() == a.size());

    std::cout << "test_rule_of_five_copy passed\n";
}

// ---------------------------------------------------------------------
// 4. Rule of Five (Move)
// Confirms that move does not allocate new memory and only transfers the pointer
// ---------------------------------------------------------------------
void test_rule_of_five_move() {
    Conversation a;
    a.append(Message(Role::User, "one"));
    a.append(Message(Role::User, "two"));

    const Message* original_ptr = a.begin();
    std::size_t original_size = a.size();

    Conversation b(std::move(a));  // move constructor
    assert(b.begin() == original_ptr);
    assert(b.size() == original_size);
    assert(a.size() == 0);
    assert(a.begin() == a.end());

    Conversation c;
    c.append(Message(Role::User, "x"));
    const Message* c_ptr = c.begin();

    Conversation d;
    d = std::move(c);  // move assignment
    assert(d.begin() == c_ptr);
    assert(c.size() == 0);

    std::cout << "test_rule_of_five_move passed\n";
}

// ---------------------------------------------------------------------
// 5. Growth behavior
// Confirm that capacity doubles correctly when needed
// and that functions size() and at()
// ---------------------------------------------------------------------
void test_growth_behavior() {
    Conversation c;
    const int N = 20;  // crosses several doubling boundaries: 1,2,4,8,16
    for (int i = 0; i < N; ++i) {
        c.append(Message(Role::User, "msg" + std::to_string(i)));
        assert(c.size() == static_cast<std::size_t>(i + 1));
    }
    for (int i = 0; i < N; ++i) {
        assert(c.at(i).content() == "msg" + std::to_string(i));
    }
    std::cout << "test_growth_behavior passed\n";
}

// ---------------------------------------------------------------------
// 6. Scanner (Clean Text)
// Confirms that the scanner processes strings with no sentinel 
// ---------------------------------------------------------------------
void test_scanner_clean_text() {
    SentinelScanner s("<|end_conversation|>");
    auto out = s.feed("Hello, how can I help?");
    assert(out.safe_text == "Hello, how can I help?");
    assert(!out.sentinel_found);

    auto flushed = s.flush();
    assert(flushed.safe_text.empty());
    assert(!flushed.sentinel_found);

    std::cout << "test_scanner_clean_text passed\n";
}

// ---------------------------------------------------------------------
// 7. Scanner  (Split Sentinel)
//  Confirms thatthe scanner catches the sentinel split across every possible boundary
// This is is from the example given the the P2_spec.md file 
// ---------------------------------------------------------------------
void test_scanner_split_sentinel() {
    const std::string sentinel = "<|end_conversation|>";
    const std::string text = "Goodbye." + sentinel;
    for (std::size_t split = 0; split <= text.size(); ++split) {
        SentinelScanner scanner(sentinel);
        auto out1 = scanner.feed(text.substr(0, split));
        auto out2 = scanner.feed(text.substr(split));
        assert((out1.sentinel_found || out2.sentinel_found) &&
               "sentinel must be caught regardless of split point");
        assert(out1.safe_text + out2.safe_text == "Goodbye.");
    }
    std::cout << "test_scanner_split_sentinel passed\n";
}

// ---------------------------------------------------------------------
// 8. Scanner (False Alarms)
// Ensures that the scanner doesn't trigger on partial matches
// ---------------------------------------------------------------------
void test_scanner_false_alarms() {
    const std::string sentinel = "<|end_conversation|>";

    SentinelScanner s1(sentinel);
    auto out1 = s1.feed("<|end_world|>");
    assert(!out1.sentinel_found);
    auto flush1 = s1.flush();
    assert(out1.safe_text + flush1.safe_text == "<|end_world|>");

    SentinelScanner s2(sentinel);
    auto out2a = s2.feed("<|end_");
    assert(!out2a.sentinel_found);
    auto out2b = s2.feed("nope, bye");
    assert(!out2b.sentinel_found);
    auto flush2 = s2.flush();
    assert(out2a.safe_text + out2b.safe_text + flush2.safe_text ==
           "<|end_nope, bye");

    std::cout << "test_scanner_false_alarms passed\n";
}

// ---------------------------------------------------------------------
// 9. Scanner (Bounded Memory)
// Confirms that pending_ never exceeds sentinel.size() - 1
// while feeding a large adversarial stream.
// ---------------------------------------------------------------------
void test_scanner_bounded_memory() {
    const std::string sentinel = "<|end_conversation|>";
    SentinelScanner scanner(sentinel);

    const std::string unit = "<|end_";  // sentinel's own prefix, adversarial
    const int repeats = 5000;

    std::size_t total_fed = 0;
    std::size_t total_safe = 0;

    for (int r = 0; r < repeats; ++r) {
        for (char ch : unit) {
            auto out = scanner.feed(std::string(1, ch));
            total_fed += 1;
            total_safe += out.safe_text.size();
            assert(!out.sentinel_found);

            std::size_t implied_pending = total_fed - total_safe;
            assert(implied_pending <= sentinel.size() - 1);
        }
    }

    auto flushed = scanner.flush();
    total_safe += flushed.safe_text.size();
    assert(total_safe == total_fed);

    std::cout << "test_scanner_bounded_memory passed\n";
}

// ---------------------------------------------------------------------
// 10. Harness (Turn Limit), this uses scripts/greeting.script
// Confirms that when harness runs with Conversation underneath,
// the loop stops with TurnLimit
// ---------------------------------------------------------------------
void test_harness_turn_limit() {
    HarnessConfig cfg;
    cfg.max_turns = 2;  // greeting.script's sentinel is on the 3rd block

    auto model = std::make_unique<ScriptedModelClient>("scripts/greeting.script");
    cfg.system_message = model->system_message();
    Harness harness(std::move(model), cfg);

    FakeInput in({"test", "hello"});
    CapturingOutput out;

    StopReason reason = harness.run(in, out);
    assert(reason.kind == StopReason::Kind::TurnLimit);

    std::cout << "test_harness_turn_limit passed\n";
}

// ---------------------------------------------------------------------
// 11. Harness (Sentinel Halt), this uses scripts/greeting.script
// Verifies that the provided loop halts exactly when the SentinelScanner reports the sentinel found.
// ---------------------------------------------------------------------
void test_harness_sentinel_halt() {
    HarnessConfig cfg;  // default max_turns = 20, plenty of headroom

    auto model = std::make_unique<ScriptedModelClient>("scripts/greeting.script");
    cfg.system_message = model->system_message();
    Harness harness(std::move(model), cfg);

    FakeInput in({"test", "hello", "you"});  // 3 turns to reach the sentinel
    CapturingOutput out;

    StopReason reason = harness.run(in, out);
    assert(reason.kind == StopReason::Kind::Sentinel);

    // Sentinel must never leak into what's actually printed.
    assert(out.captured().find("<|end_conversation|>") == std::string::npos);

    // But it must be present in the stored transcript (see harness.cpp's
    // top-of-file comment on why the two differ).
    assert(harness.conversation().at(harness.conversation().size() - 1).content()
           == "Goodbye!<|end_conversation|>");

    std::cout << "test_harness_sentinel_halt passed\n";
}

// ---------------------------------------------------------------------
// 12. Transcript Round-Trip
// Checks that a transcript gets parsed correctly and replayed back in order by ReplayModelClient
// ---------------------------------------------------------------------
void test_transcript_round_trip() {
    const char* path = "test_transcript_roundtrip.txt";
    std::ofstream f(path);
    f << "role: system\n"
         "Be concise.\n"
         "---\n"
         "role: user\n"
         "hello\n"
         "---\n"
         "role: assistant\n"
         "Hi! What can I do for you today?\n"
         "---\n"
         "role: user\n"
         "goodbye\n"
         "---\n"
         "role: assistant\n"
         "Goodbye.<|end_conversation|>\n";
    f.close();

    ReplayModelClient model(path);
    assert(model.system_message() == "Be concise.");

    Conversation dummy;  // ReplayModelClient ignores the conv argument

    Message reply1 = model.generate(dummy);
    assert(reply1.role() == Role::Assistant);
    assert(reply1.content() == "Hi! What can I do for you today?");

    Message reply2 = model.generate(dummy);
    assert(reply2.role() == Role::Assistant);
    assert(reply2.content() == "Goodbye.<|end_conversation|>");

    std::remove(path);
    std::cout << "test_transcript_round_trip passed\n";
}

// ---------------------------------------------------------------------
int main() {
    test_empty_conversation_bounds();
    test_system_message_ordering();
    test_harness_pins_system_message_first();
    test_rule_of_five_copy();
    test_rule_of_five_move();
    test_growth_behavior();
    test_scanner_clean_text();
    test_scanner_split_sentinel();
    test_scanner_false_alarms();
    test_scanner_bounded_memory();
    test_harness_turn_limit();
    test_harness_sentinel_halt();
    test_transcript_round_trip();

    std::cout << "All tests passed!\n";
    return 0;
}