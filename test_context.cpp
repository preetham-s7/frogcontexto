#include "frogcontexto/context.hpp"

#include <cassert>
#include <iostream>

int main() {
    const std::string checkpoint_text = R"(# FrogContexto Checkpoint

objective: Build FrogContexto
status: In progress

## Decisions
- Use C++20
- Store Markdown checkpoints

## Completed
- Define the data model

## Pending
- Add API adapter

## Confirmed Facts
- The project is self-contained

## Open Questions
- Which runtime owns compaction?
)";

    const std::string current_text = R"(# FrogContexto Checkpoint

objective: Build FrogContexto
status: Complete

## Decisions
- Use C++20
- Store Markdown checkpoints

## Completed
- Define the data model
- Implement the checkpoint store

## Pending

## Confirmed Facts
- The project is self-contained

## Open Questions
- Which runtime owns compaction?
)";

    const auto checkpoint = frogcontexto::parse_markdown(checkpoint_text);
    const auto current = frogcontexto::parse_markdown(current_text);
    assert(checkpoint.objective == "Build FrogContexto");
    assert(checkpoint.decisions.size() == 2);
    assert(current.completed.size() == 2);

    const auto result = frogcontexto::reconcile(checkpoint, current);
    assert(result.confirmed.size() == 5);
    assert(result.missing.size() == 1);
    assert(result.new_items.size() == 1);
    assert(result.conflicts.size() == 1);
    assert(result.next_action == "Resolve conflicts before continuing.");

    const auto rendered = frogcontexto::render_markdown(checkpoint, "test");
    const auto round_trip = frogcontexto::parse_markdown(rendered);
    assert(round_trip.objective == checkpoint.objective);
    assert(round_trip.pending == checkpoint.pending);

    std::cout << "test_context: PASS\n";
    return 0;
}
