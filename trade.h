/*
 * trade.h - Trading and commerce system interface definitions
 *
 * This header file provides the interface definitions and constants for the
 * comprehensive trading and commerce system implemented in trade.c. The trading
 * system enables players to exchange commodities, military units, land, and
 * resources through an auction-based marketplace with persistent file storage.
 *
 * TRADING SYSTEM ARCHITECTURE:
 * ============================
 *
 * COMMODITIES EXCHANGE FRAMEWORK:
 * ┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
 * │ Interactive     │───▶│ File-Based       │───▶│ Administrative  │
 * │ Trading         │    │ Marketplace      │    │ Processing      │
 * │ Interface       │    │ Storage          │    │ System          │
 * └─────────────────┘    └──────────────────┘    └─────────────────┘
 *
 * SUPPORTED COMMODITY TYPES:
 * - Basic Resources: Gold (currency), Food, Metal/Iron, Jewels
 * - Territory: Land sectors with varying vegetation and productivity
 * - Military Assets: Army units (mercenaries, siege, creatures), Naval fleets
 * - Special Items: God/NPC merchant goods at fixed exchange rates
 *
 * TRADING OPERATIONS SUPPORTED:
 * - BUY: Purchase commodities from other players or NPC merchants
 * - SELL: List commodities for sale with minimum price requirements
 * - UNSELL: Remove own commodities from marketplace
 * - AUCTION: Automated bid processing with highest bidder wins
 *
 * MARKETPLACE MECHANICS:
 * ┌────────────────┬─────────────────┬──────────────────┬─────────────────┐
 * │ Operation      │ Player Action   │ File Storage     │ Turn Processing │
 * ├────────────────┼─────────────────┼──────────────────┼─────────────────┤
 * │ List for Sale  │ Reserve items   │ SELL entry       │ Display market  │
 * │ Place Bid      │ Reserve payment │ BUY entry        │ Compare bids    │
 * │ Cancel Sale    │ Return items    │ NOSALE entry     │ Remove listing  │
 * │ Execute Trade  │ N/A (automatic) │ Transfer assets  │ Highest bid wins│
 * └────────────────┴─────────────────┴──────────────────┴─────────────────┘
 *
 * ECONOMIC INTEGRATION:
 * - Trading Costs: 20% transaction fee (80% efficiency) for market operations
 * - Resource Validation: Ownership and availability checks before transactions
 * - Balance Prevention: Reserved commodities cannot be used elsewhere
 * - Fair Market: Auction system ensures competitive pricing
 *
 * ADMINISTRATIVE FUNCTIONS:
 * - Nation Cleanup: Remove all trade items when nations are eliminated
 * - Turn Processing: Execute all pending trades during turn updates
 * - Error Recovery: Handle trade failures with automatic reversals
 * - Notification System: Generate news reports and mail messages
 *
 * PERSISTENT STORAGE SYSTEM:
 * The trading system uses file-based storage (tradefile) with the following format:
 * - Seven fields per entry: deal_type nation_id commodity1 commodity2 value1 value2 extra
 * - Transaction Types: SELL (listing), BUY (bid), NOSALE (cancellation)
 * - Marketplace State: Preserved across game sessions and turn boundaries
 * - Atomic Operations: Trade file updates ensure consistency
 *
 * COMPILATION DEPENDENCIES:
 * This trading system is conditionally compiled with:
 * - #ifdef TRADE: Core trading functionality compilation guard
 * - #ifdef CONQUER: Player-facing interface functions
 * - #ifdef ADMIN: Administrative and turn processing functions
 *
 * INTEGRATION WITH GAME SYSTEMS:
 * - Nation Management: Resource totals, army/navy rosters, land ownership
 * - Display System: Full-screen curses interface for marketplace
 * - Communication: News generation and mail notification system
 * - Turn Processing: Automated trade execution during turn updates
 * - Validation: Army tradability, land ownership, resource availability
 *
 * This file is part of Conquer.
 * Originally Copyright (C) 1988-1989 by Edward M. Barlow and Adam Bryant
 * Copyright (C) 2025 Juan Manuel Méndez Rey (Vejeta) - Licensed under GPL v3 with permission from original authors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TRADE_H
#define TRADE_H

/*
 * ADMINISTRATIVE TRADE MANAGEMENT FUNCTIONS
 * =========================================
 *
 * These functions provide administrative control over the trading system,
 * used primarily during turn processing and nation management operations.
 */

/*
 * fixtrade - Remove all trade items belonging to specified nation (ADMIN only)
 *
 * Administrative function to remove all commodities that a nation has listed
 * for sale from the trading marketplace. Used when a nation is eliminated,
 * goes inactive, or requires administrative cleanup of their trade presence.
 * Ensures marketplace integrity by removing invalid or unavailable items.
 *
 * Operation Process:
 * 1. Reads current trade file to identify nation's active sales
 * 2. Processes only SELL entries belonging to specified nation
 * 3. Generates NOSALE entries to cancel each item
 * 4. Returns reserved commodities to nation via takeback()
 * 5. Maintains trade file consistency for other nations
 *
 * Administrative Use Cases:
 * - Nation elimination or conquest cleanup
 * - Inactive player account management
 * - Game balance corrections and interventions
 * - Database consistency maintenance
 *
 * Parameters:
 *   cntry - Nation index whose trade items should be removed from marketplace
 *
 * Returns: void
 *
 * Side Effects:
 *   - Modifies trade file with NOSALE entries for nation's items
 *   - Returns reserved commodities to nation via takeback()
 *   - Removes nation's presence from active marketplace
 *   - May terminate program on file I/O errors
 *
 * Testing Notes:
 *   Category: C (System) - Requires file system and complete game state
 *   Approach: System testing with mock trade files and nation data
 *   Key Tests: Item identification, file updates, commodity restoration
 *   Dependencies: Trade file, nation data, takeback() function
 *   Mock Requirements: File system, trade file format, complete nation state
 *   Complexity: Moderate - File processing with selective updates
 *
 * Compilation Requirements:
 *   - Requires TRADE and ADMIN preprocessor definitions
 *   - Available only in administrative build configurations
 *   - Not accessible to regular player interface
 *
 * Notes:
 *   - ADMIN-only function for administrative and cleanup operations
 *   - Essential for maintaining marketplace integrity
 *   - Used during nation elimination or administrative intervention
 *   - Prevents orphaned trade entries from inactive nations
 */
void fixtrade(int cntry);

/*
 * RELATED TRADING SYSTEM FUNCTIONS
 * ================================
 *
 * The complete trading system includes additional functions implemented
 * in trade.c but not exposed through this header interface:
 *
 * USER INTERFACE FUNCTIONS (CONQUER builds):
 * - trade(): Main interactive trading interface
 * - tradeerr(): Error display with user acknowledgment
 * - checkland(): Land sector validation for trading
 * - getland(): Interactive vegetation selection interface
 * - gettrade(): Commodity type selection interface
 * - tradable(): Army unit tradability validation
 *
 * CORE TRADING MECHANICS:
 * - setaside(): Reserve commodities for active trading
 * - takeback(): Return reserved commodities to available pool
 * - armyvalue(): Calculate commercial value of army units
 * - checktrade(): Update player trade status from file
 *
 * ADMINISTRATIVE FUNCTIONS (ADMIN builds):
 * - tradeit(): Execute commodity transfers between nations
 * - gettval(): Calculate trade values for bid comparison
 * - trademail(): Send trade completion notifications
 * - uptrade(): Process all trades during turn update
 *
 * MARKETPLACE DATA FLOW:
 * User Interface → File Storage → Turn Processing → Notifications
 *      ↓               ↓              ↓              ↓
 *   trade()      tradefile      uptrade()     trademail()
 * gettrade()        ↕          tradeit()        ↕
 * checkland()   setaside()     gettval()     News System
 *
 * INTEGRATION REQUIREMENTS:
 * - Nation data structures for resource management
 * - Army/navy data structures for unit trading
 * - Map data structures for land trading
 * - File I/O system for persistent marketplace storage
 * - Display system for user interface (curses library)
 * - News and mail systems for trade notifications
 *
 * TESTING STRATEGY FOR TRADING SYSTEM:
 * - Unit Tests: Individual function validation with mock data
 * - Integration Tests: Multi-function workflows with test data
 * - System Tests: Complete trading scenarios with file persistence
 * - Performance Tests: Large marketplace and concurrent access
 * - Error Tests: File corruption, resource shortage, validation failures
 */

#endif /* TRADE_H */