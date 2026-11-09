# Prime Counting: π(10^13)

Count the number of prime numbers `p` in the range `2 ≤ p < 10^13`
(ten trillion).

## Deliverable

Write **only the integer count** as ASCII digits to `/workspace/answer.txt`.
No thousands separators, no leading/trailing whitespace, no other text.

Example: if you were asked for π(100), you would write
```
25
```
to `/workspace/answer.txt`.

## Rules

- Container: 2 CPU / 4 GiB RAM.
- `apt-get` is available; no internet after container start.
- The verifier checks the exact integer value in `/workspace/answer.txt`.
- Expected runtime with the `primesieve` library CLI on 2 cores:
  **10-25 minutes**. A naive Python sieve of Eratosthenes will not
  finish within the agent timeout budget.
- The primesieve library (BSD-2-Clause) is installed in the base image
  (`apt install primesieve` was done during build); the `primesieve`
  binary is on `$PATH`.

## Reference

π(10^13) is a known mathematical constant. See
[Prime-counting function on Wikipedia](https://en.wikipedia.org/wiki/Prime-counting_function)
for π(10^N) up to N=27.
