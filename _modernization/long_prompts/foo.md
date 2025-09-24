  Complete File Warning Analysis - Phase 4.8 Todo List

  HIGHEST PRIORITY TARGETS (Most warnings - tackle these next)

  | Rank | File       | Warning Count | Compilation Mode | Priority    | Status        |
  |------|------------|---------------|------------------|-------------|---------------|
  | 1    | makeworl.c | ~83           | Admin-only       | 🔴 CRITICAL | ⏳ NEXT TARGET |
  | 2    | navy.c     | ~74           | Dual-compiled    | 🔴 Critical | ⏳ Pending     |
  | 3    | misc.c     | ~69           | Dual-compiled    | 🔴 Critical | ⏳ Pending     |
  | 4    | newlogin.c | ~68           | Admin-only       | 🔴 Critical | ⏳ Pending     |
  | 5    | cexecute.c | ~64           | Dual-compiled    | 🔴 Critical | ⏳ Pending     |

  MEDIUM PRIORITY TARGETS (High warning counts)

  | Rank | File       | Warning Count | Compilation Mode | Priority | Status    |
  |------|------------|---------------|------------------|----------|-----------|
  | 6    | update.c   | ~58           | Admin-only       | 🟡 High  | ⏳ Pending |
  | 7    | npc.c      | ~58           | Admin-only       | 🟡 High  | ⏳ Pending |
  | 8    | magic.c    | ~58           | Dual-compiled    | 🟡 High  | ⏳ Pending |
  | 9    | io.c       | ~56           | Dual-compiled    | 🟡 High  | ⏳ Pending |
  | 10   | reports.c  | ~48           | Game-only        | 🟡 High  | ⏳ Pending |
  | 11   | commands.c | ~47           | Game-only        | 🟡 High  | ⏳ Pending |
  | 12   | move.c     | ~43           | Game-only        | 🟡 High  | ⏳ Pending |
  | 13   | randeven.c | ~40           | Admin-only       | 🟡 High  | ⏳ Pending |
  | 14   | main.c     | ~35           | Game-only        | 🟡 High  | ⏳ Pending |
  | 15   | forms.c    | ~31           | Game-only        | 🟡 High  | ⏳ Pending |

  COMPLETED AND PARTIALLY COMPLETED

  | File     | Original → Current | Status          | Completion |
  |----------|--------------------|-----------------|------------|
  | combat.c | 76 → 6             | ✅ 92% COMPLETE  | Excellent  |
  | admin.c  | 36 → 9             | 🔄 75% COMPLETE | Good       |
  | trade.c  | 23 → 0             | ✅ COMPLETE      | Perfect    |
  | spew.c   | 16 → 0             | ✅ COMPLETE      | Perfect    |

  LOW PRIORITY TARGETS (Manageable warning counts)

  | File      | Warning Count | Compilation Mode | Priority   |
  |-----------|---------------|------------------|------------|
  | extcmds.c | ~14           | Game-only        | 🟢 Medium  |
  | display.c | ~8            | Game-only        | 🟢 Low     |
  | psmap.c   | ~6            | PostScript       | 🟢 Low     |
  | check.c   | 0             | Shared           | ✅ Complete |
  | data.c    | 0             | Dual-compiled    | ✅ Complete |

  STRATEGIC NEXT STEPS

  Based on this analysis, you mentioned your grep command showed the largest number of warnings. I can see that makeworl.c with ~83 warnings is indeed
  the largest target remaining.

  Recommended Phase 4.8C Implementation Order:

  1.
  1. Begin Phase 4.8C with makeworl.c (83 warnings - largest target)
  2. Progress through navy.c (74 warnings - dual-compiled complexity)
  3. Continue with misc.c (69 warnings - dual-compiled)
  5. Tackle newlogin.c (68 warnings - admin-only)
