# AI Mode Test Checklist

## Functional

- [ ] Start AI game as white.
- [ ] Start AI game as black (AI makes first move).
- [ ] Start AI game with random color multiple times.
- [ ] Verify `AI_CHALLENGE_SENT` then `MATCH_STARTED` order.
- [ ] Verify `MATCH_STARTED` includes `opponent_is_ai` and `ai_depth`.

## Move Flow

- [ ] Human move receives `MOVE_ACCEPTED`.
- [ ] AI response arrives via `OPPONENT_MOVE`.
- [ ] `OPPONENT_MOVE` includes `ai_think_ms` and `ai_nodes_searched`.
- [ ] Board and turn stay consistent after 20+ plies.

## Failure Paths

- [ ] AI timeout emits error `AI_TIMEOUT`.
- [ ] AI no-move emits error `AI_NO_MOVE`.
- [ ] Timeout/no-move ends game with reason `ai_timeout` or `ai_no_move`.

## Endgame

- [ ] Checkmate ends game with uppercase result (`WHITE_WIN` / `BLACK_WIN`).
- [ ] Draw scenarios end with result `DRAW`.
- [ ] Move history and duration are present in `GAME_ENDED`.

## Performance

- [ ] Easy: typical think time < 300ms.
- [ ] Medium: typical think time < 1200ms.
- [ ] Hard: typical think time < 2500ms (depends on host).
