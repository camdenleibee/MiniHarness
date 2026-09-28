# Design Log — Project 2

## Growth factor and amortized cost

`Conversation::append` allocates more space for the array by doubling it: whenever
`size_ == capacity_`, the new capacity is `capacity_ == 0 ? 1 :
capacity_ * 2`. By choosing doubling it makes `append` amortized
O(1) rather than O(n) per call.

Proof: if we consider N total `append` calls, starting from an empty
Conversation. Reallocation will only happen when the array is exactly
full, so at sizes 1, 2, 4, 8, ..., up to the final capacity C. 
Each reallocation of size k copies k existing elements. Summing the
cost of every reallocation gives the
geometric series 1 + 2 + 4 + ... + C, which is at most 2C - 1 —
bounded by a constant multiple of C, and by the rules of Big O notation the
total copying work across N appends is O(N). Which means  the *average* cost per
append is O(1). If the growth was instead by a fixed +1 each time:
reallocation would happen every single append, copying 1, 2, 3, ...,
N elements respectively, for a total of N(N+1)/2 = O(N²).

## Rule of Five evidence

All five special member functions are implemented in
`src/conversation.cpp`:

- **Destructor** calls `delete[] data_`. Since `delete[] nullptr` is
  well-defined as a no-op, this is safe even on a moved-from object.
- **Copy constructor / copy assignment** allocates an entirely new
  buffer (`new Message[capacity_]`) and deep-copy every element, so
  `this->begin() != other.begin()` afterward - verified directly in
  test #3. Copy assignment also guards against self-assignment
  (`if (this == &other) return *this;`) before freeing its own buffer,
  since without that guard for `c = c;` it would delete the data it is 
  about to read from.
- **Move constructor / move assignment** steals the source's `data_`
  pointer directly with no per-element copying. Then it zeroes out the source's
  `data_`/`size_`/`capacity_` so it's left in a empty,
  safely-destructible state. Test #4 also confirms the destination's
  `begin()` equals the source's *original* pointer (true theft, not a
  copy) and that the source is zeroed afterward.

The full test suite and the interactive `miniharness` run were
both built and both executed without any zero leaks or double-frees
reported.

## Sentinel scanner: bounded pending_ proof

Invariant: `pending_.size() <= sentinel_.size() - 1` at all times.

Argument: on every `feed()` call, `working = pending_ + chunk` is
searched for a full occurrence of `sentinel_`. If this is found, the function
returns immediately with `sentinel_found = true` and clears
`pending_` to empty — so that `pending_` is never left holding a true
match. If no full match is found, the function computes the longest
suffix of `working` that matches a *prefix* of `sentinel_`, and that
suffix becomes the new `pending_`. A partial match of a
string of length L can be at most L-1 characters, since if it were L
characters, it would be the sentinel itself and would have been caught
by the `find()` check first, and not treated as a partial match. So
`pending_` can never hold more than `sentinel_.size() - 1` characters.

This is what keeps memory usage O(sentinel length) rather than
O(stream length): the scanner never accumulates the whole reply, only
the trailing fragment that might still complete a match.
Test #9 verifies this by feeding an adversarial stream,
(the sentinel's own prefix, repeated thousands of times, one byte at a time)
and tracking`total_fed - total_safe` as an implied measure of how much is
currently held back; this value never exceeds `sentinel.size() - 1`
across the entire stream.

## What I would change differently

The key change I would make is actually plugging in a free opensource
LLM into this project to see the outcome. It would be nice to have a 
truly working harness that pulls data from a LLM and to see if it works.
This would also help me understand API's and working with LLM in a program, 
which is a skill that could be very useful.