1. ✅ No complex locking
2. ✅ Handles thousands of connections efficiently
3. ✅ Simpler than multi-process
4. ✅ Perfect for I/O-bound operations like network gaming

## Message Flow Example
```
User A (Thread/Task 1)          Server          User B (Thread/Task 2)
       |                          |                      |
       |----login---------------->|                      |
       |                    [store socket_A]             |
       |                          |<------login----------|
       |                    [store socket_B]             |
       |                          |                      |
       |--challenge(user_B)------>|                      |
       |                    [lookup socket_B]            |
       |                          |--challenge_msg------>|
       |                          |                      |
       |                          |<--accept-------------|
       |<---match_started---------|                      |
       |                          |--match_started------>|