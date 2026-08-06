# Fusion Binary Protocol

Observations that I believe are probably correct


## Encoding

Fusion minimizes bandwidth by writing to a continuous bitstream that ignores byte alignment. At the core of its compression is a 6-bit chunked Variable-Length Integer (Var-Int) encoding, where every 6 bits of payload data are followed by a 1-bit continuation flag indicating if more chunks are required. To prevent negative numbers from maximizing the chunk count, all signed integers are first mapped to positive values using ZigZag encoding before being packed into Var-Ints. Floating-point numbers are handled via fixed-point quantization; they are multiplied by a scale factor (typically 256), rounded to the nearest integer, and then written to the stream just like any other signed integer. As an exception to this aggressive bit-packing, strings are written as raw plaintext bytes, immediately preceded by a standard 32-bit length prefix.


## Header

### Reliability information

The entire buffer seems to have a maximum size of `1136` bytes. This is probably to stay within MTU (`1200` bytes here) and avoid ENet fragmentation.

 1. `payload[0]:op` -> 8U: Operation code
 2. `payload[1]:frag` -> 8U: Fragment index, `0x00` if not fragmented, OR'd with `0x80` to mark last fragment
 3. `payload[2..3]:send_seq` -> U16LE: Own sequence number, starts counting at `0x01`
 4. `payload[4..5]:recv_high` -> U16LE: Highest peer sequence number received
 5. `payload[6..13]:recv_mask` -> U64LE: bitset that filled via `previous << 1 | bit` where `bit == 1` represents a peer sequence number, where the LSB always represents the highest peer sequence number received

- `recv_high` is the highest peer sequence number being acknowledged.
- `recv_mask` is a **64-bit little-endian ack bitmap**.
- Bit 0 acknowledges `recv_high`
- Bit 1 acknowledges `recv_high - 1`
- Bit 2 acknowledges `recv_high - 2`
- etc.

For example, if the peers current sequence number is `143` and the servers current sequence number is `432`, but the peer has just received the new sequence number from the server after missing 2 previous ones, a `0x03` operation from the peer would look like this:

 1. `op` = 3
 2. `frag` = 0
 3. `send_seq` = 143
 4. `recv_high` = 432
 5. `recv_mask` = 0b1111111111111111111111111111111111111111111111111111111111111001

The sequence number sent is constant for each burst of fragments.

After the header, (possibly fragmented) operation data seems to follow. Does each kind of operation begins with the same "operation header"? Or does each kind of operation have its own "operation header"?


## Operations

### Event `101` Operation `0x03` (State Update / Delta Stream)

This appears to be the primary packet type carrying the simulation state. The 14-byte reliability header is immediately followed by a strictly ordered, heavily bit-packed sequence of structures.

#### 1. Simulation Packet Header (8 bytes)

The first 8 bytes of the payload map exactly to the simulation packet header. It is written at bit-offset 112 (byte 14) as a raw 64-bit unsigned integer:

 1. `data[0]:message_count` -> 8U: Number of RPCs or internal simulation messages
 2. `data[1]:inputs_or_cells` -> 8U: In Client/Server topologies: number of inputs, in Shared Mode with area of interest: cells
 3. `data[2]:update_count` -> 8U: Number of object state updates in this packet
 4. `data[3]:destroy_count` -> 8U: Number of objects destroyed in this packet
 5. `data[4..7]:simulation_tick` -> 32S: Simulation tick


#### 2. Server Time Feedback (Server-to-Client only)

If the packet is sent by the server (or the Master Client relay in Shared Mode), the packet header is immediately followed by time feedback data. This is used to synchronize the client's clock and prediction buffers. It consists of 4 floating-point values compressed into variable-length integers (Var-Ints):

* `offset_avg` (Compressed Float)
* `offset_dev` (Compressed Float)
* `recv_delta_avg` (Compressed Float)
* `recv_delta_dev` (Compressed Float)

*Compression math:* Each float is quantized by multiplying it by 256, cast to an integer, ZigZag-encoded, and written to the bitstream as a 6-bit chunked Var-Int.


### Event `101` Operation `0x04` (Standalone ACK)

Contains *just* the Reliability information header, but the own sequence number is zero. Sending this operation also doesn't increment the own sequence number.

The other side often resumes with the next burst right after one of these are sent. This means that if one side currently has no operations to send that would acknowledge the last operations from the other side, this is sent instead.
