# mad
Misha's Awesome Diff

This is limited to 2\*\*32 bytes of the source and target combined, so each can be 2GB. This is both because it'd use more than 4GB RAM to calculate the diff, and because bigger usecases shouldn't use my silly algorithm.

Our source string is S, and our target string is T.
It has two operations - COPY and ADD.

- ADD, encoded as 0, has a number n (up to 127 because you can just do another add) after it, and then n bytes. These bytes are added directly. Therefore the worst encoding will just have an extra byte for every 127 bytes.
- COPY, encoded as 1, gets a number (up to 127 because you can just do another copy) n and a 32 bit address i. It copies n bytes from the address i in (S+T). This allows repetitions to be represented as a copy that overlaps itself.

Applying a diff is trivial - you just follow the instructions. To calculate a diff, for every string you have, you need to find the longest sequence before it which is equal.

To do that efficiently, we first calculate a rolling hash of every 6, 16, 32 and 128 bytes sequence in S. Then, we check if the current 128 bytes hash appeared previously - if it did, find the longest of these matches. And similarly for 32, 16, and 6 bytes. I chose 6 bytes as the minimum because a COPY is 5 bytes, so anything shorter can just be an ADD.
