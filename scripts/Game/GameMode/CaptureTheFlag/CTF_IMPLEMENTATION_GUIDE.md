# CRF Capture the Flag (CTF) Gamemode - Complete Implementation Guide

## Table of Contents
1. [Overview](#overview)
2. [Features](#features)
3. [Prerequisites](#prerequisites)
4. [Quick Start Guide](#quick-start-guide)
5. [Detailed Implementation](#detailed-implementation)
6. [Entity Setup Requirements](#entity-setup-requirements)
7. [Component Configuration](#component-configuration)
8. [Prefab Creation](#prefab-creation)
9. [Testing and Validation](#testing-and-validation)
10. [Troubleshooting](#troubleshooting)
11. [Advanced Configuration](#advanced-configuration)
12. [Performance Optimization](#performance-optimization)
13. [Technical Reference](#technical-reference)

## Overview
The CRF CTF Gamemode Component implements a classic Capture the Flag gamemode for the Coalition Reforger Framework. Teams must capture a central flag and bring it to their team's drop-off zone, holding it there for a configurable duration to achieve victory.

### Gamemode Flow
1. **Flag spawns** at a central location marked by the `FlagSpawn` entity
2. **Players** from any enabled faction can pick up the flag
3. **Flag carrier** must bring the flag to one of their faction's drop zones
4. **Capture timer** starts when flag carrier enters a friendly drop zone
5. **Victory** is achieved when timer completes (default: 5 minutes)
6. **Game resets** automatically for the next round

## Features
- ✅ **Capturable Flag**: Central flag item that players can pick up and carry
- ✅ **Multi-Team Drop Zones**: Two drop-off zones per participating faction
- ✅ **Flexible Faction Support**: Enable/disable BLUFOR, OPFOR, and INDFOR participation
- ✅ **Configurable Timers**: Adjustable capture duration and marker update intervals
- ✅ **Dynamic Map Markers**: Real-time flag position tracking
- ✅ **Network Synchronized**: Full multiplayer support with proper replication
- ✅ **Victory Integration**: Integrates with CRF gamemode victory system
- ✅ **Visual Feedback**: Real-time UI updates and notifications
- ✅ **Admin Controls**: Admin panel integration for gamemode management
- ✅ **Respawn Integration**: Handles flag drops on player death/disconnect

## Prerequisites

### Required Software
- **Arma Reforger Workbench** (latest stable version)
- **Coalition Reforger Framework** (this mod loaded as dependency)
- **Text editor** (VS Code, Notepad++, etc.) for configuration

### Required Knowledge
- Basic Arma Reforger Workbench usage
- Entity placement and world editing
- Understanding of component systems
- Basic prefab creation concepts

## Quick Start Guide

### 🚀 Minimal Setup (5 Minutes)

1. **Add CTF Component**
   - Find your Game Mode entity in the world
   - Add `CRF_CTFGamemodeManager` component
   - Configure basic settings (factions, timers)

2. **Place Required Entities**
   ```
   Required Entity Names (case-sensitive):
   - FlagSpawn (always required)
   - BluforDropZone1, BluforDropZone2 (if BLUFOR enabled)
   - OpforDropZone1, OpforDropZone2 (if OPFOR enabled)  
   - IndforDropZone1, IndforDropZone2 (if INDFOR enabled)
   ```

3. **Set Prefab Paths**
   - Flag Prefab: Use provided CRF flag prefab or create custom
   - Drop Zone Prefab: Use provided CRF drop zone prefab or create custom

4. **Test**
   - Launch test session
   - Verify flag spawns and drop zones work
   - Test flag pickup and capture mechanics

### ⚡ Default Configuration
```
Enabled Factions: BLUFOR + OPFOR
Capture Timer: 300 seconds (5 minutes)
Marker Updates: Every 120 seconds (2 minutes)
Zone Radius: 25 meters
```

## Detailed Implementation

### Step 1: World Preparation

1. **Load Dependencies**
   - Ensure Coalition Reforger Framework is loaded
   - Verify mod compatibility and load order
   - Check for version conflicts

2. **Plan Your Layout**
   - **Central flag area**: Equidistant from team bases
   - **Team bases**: Where drop zones will be placed
   - **Travel routes**: Consider balance and engagement opportunities
   - **Terrain considerations**: Avoid inaccessible or problematic areas

3. **Faction Planning**
   - Decide which factions will participate
   - Plan spawn locations relative to drop zones
   - Consider force balance and team sizes

### Step 2: Gamemode Component Setup

1. **Locate Game Mode Entity**
   - Usually named "Game Mode" or similar in world hierarchy
   - Should be at root level of world entities
   - Must be the primary gamemode controller entity

2. **Add CTF Component**
   - Right-click Game Mode entity → "Add Component"
   - Search for `CRF_CTFGamemodeManager`
   - Add component to entity

3. **Initial Configuration**
   ```
   Component Settings:
   ✓ BLUFOR Enabled: true (recommended)
   ✓ OPFOR Enabled: true (recommended)  
   ✓ INDFOR Enabled: false (optional)
   ✓ Capture Timer: 300 (5 minutes)
   ✓ Marker Update Interval: 120 (2 minutes)
   ✓ Hide Map Markers: false
   ```

### Step 3: Entity Placement (Critical)

### Step 3: Entity Placement (Critical)

⚠️ **IMPORTANT**: Entity names are case-sensitive and must match exactly!

#### Required Entities

| Entity Name | When Required | Purpose |
|-------------|---------------|---------|
| `FlagSpawn` | Always | Initial flag spawn location |
| `BluforDropZone1` | When BLUFOR enabled | Primary BLUFOR capture zone |
| `BluforDropZone2` | When BLUFOR enabled | Secondary BLUFOR capture zone |
| `OpforDropZone1` | When OPFOR enabled | Primary OPFOR capture zone |
| `OpforDropZone2` | When OPFOR enabled | Secondary OPFOR capture zone |
| `IndforDropZone1` | When INDFOR enabled | Primary INDFOR capture zone |
| `IndforDropZone2` | When INDFOR enabled | Secondary INDFOR capture zone |

#### Placement Guidelines

**Flag Spawn (`FlagSpawn`)**:
- Place at strategic central location
- Should be roughly equidistant from all team bases
- Avoid placing in spawn protection areas
- Consider cover and approach routes
- Can be any entity type (invisible trigger recommended)

**Drop Zones (`[Faction]DropZone[1-2]`)**:
- Place near but not inside team spawn areas
- Maintain 200+ meter spacing between opposing zones
- Ensure adequate maneuvering space (25m radius default)
- Consider defensive positions and sight lines
- Can be any entity type (trigger recommended)

#### Step-by-Step Placement

1. **Create FlagSpawn Entity**
   ```
   Entity → Create → General → Trigger (or any entity type)
   Name: FlagSpawn
   Position: Central map location
   ```

2. **Create Drop Zone Entities**
   - For each enabled faction, create both drop zones
   - Use descriptive positions (near main base, forward outpost, etc.)
   - Ensure proper naming convention

3. **Verify Placement**
   - Check distances between zones for balance
   - Test accessibility from all angles
   - Confirm no overlapping with spawn protection

### Step 4: Prefab Configuration

#### Option A: Use Provided Prefabs (Recommended)
The CRF framework includes ready-to-use prefabs:
- **Flag Prefab**: `{CRF_GUID}Prefabs/Items/CRF_CTF_Flag.et`
- **Drop Zone Prefab**: `{CRF_GUID}Prefabs/Triggers/CRF_CTF_DropZone.et`

#### Option B: Create Custom Prefabs

**Creating Custom Flag Prefab**:
1. **New Prefab** → Name: `YourMod_CTF_Flag.et`
2. **Add Components**:
   - `MeshObject` (visual representation)
   - `CRF_CTFFlagComponent` (core mechanics)
   - `ActionsManagerComponent` (player interaction)
3. **Configure ActionsManagerComponent**:
   - Add `CRF_CTFPickupFlagAction`
   - Set interaction text: "Pick up Flag"
   - Set range: 3.0 meters
4. **Save and test**

**Creating Custom Drop Zone Prefab**:
1. **New Prefab** → Name: `YourMod_CTF_DropZone.et`
2. **Add Components**:
   - `CRF_CTFDropZoneComponent` (core mechanics)
   - `MeshObject` (visual marker - optional)
3. **Configure CTFDropZoneComponent**:
   - Set zone radius: 25.0 meters
   - Faction will be set automatically by gamemode
4. **Save and test**

### Step 5: Component Configuration

Select your Game Mode entity and configure the CTF component:

#### Faction Settings
```
Faction Configuration:
☐ BLUFOR Enabled: Enable for NATO/West forces
☐ OPFOR Enabled: Enable for Eastern forces  
☐ INDFOR Enabled: Enable for Independent forces

Recommended Combinations:
✓ 2-Team: BLUFOR + OPFOR
✓ 3-Team: BLUFOR + OPFOR + INDFOR
✗ 1-Team: Not supported (minimum 2 factions required)
```

#### Timer Configuration
```
Capture Timer: 300 (seconds)
├─ Standard: 300 (5 minutes)
├─ Quick: 120 (2 minutes)  
├─ Extended: 600 (10 minutes)
└─ Custom: 60-1800 range

Marker Update Interval: 120 (seconds)
├─ Frequent: 60 (1 minute) - Higher server load
├─ Standard: 120 (2 minutes) - Recommended
├─ Infrequent: 300 (5 minutes) - Lower server load
└─ Off: 0 (disabled) - No map markers
```

#### Prefab Paths
```
Flag Prefab: "{GUID}Prefabs/Items/CRF_CTF_Flag.et"
Drop Zone Prefab: "{GUID}Prefabs/Triggers/CRF_CTF_DropZone.et"

Notes:
- Use full resource browser paths
- Verify prefabs exist and load correctly
- Test prefabs in isolation before full implementation
```

#### Advanced Settings
```
Hide Map Markers: false (recommended)
├─ false: Show flag position on map (recommended)
├─ true: Hide flag markers (hardcore mode)

Zone Detection Method: Automatic
├─ Uses player position relative to drop zone center
├─ Accounts for zone radius setting
├─ Checks faction membership automatically
```

### Step 6: Testing and Validation

#### Pre-Launch Checklist
- [ ] CTF component added to Game Mode entity
- [ ] All required entities placed with correct names
- [ ] Faction settings match your scenario requirements  
- [ ] Timer values are reasonable (60+ seconds recommended)
- [ ] Prefab paths are valid and load successfully
- [ ] No conflicting gamemode components present

#### Basic Functionality Test
1. **Launch solo test session**
2. **Check flag spawn**:
   - Flag appears at FlagSpawn location
   - Flag has pickup interaction available
   - Flag visual model displays correctly
3. **Test flag pickup**:
   - Player can interact with flag
   - Flag attaches to player correctly
   - Map marker updates (if enabled)
4. **Test drop zones**:
   - Drop zones spawn at correct locations
   - Zone detection works when carrying flag
   - Capture timer starts in friendly zone
   - Timer progresses and completes properly

#### Multiplayer Validation
1. **Test with 2+ players** from different factions
2. **Verify network synchronization**:
   - Flag pickup replicates to all clients
   - Drop zone entry/exit syncs properly
   - Timer progression shows correctly for all players
   - Victory conditions work for all clients
3. **Test edge cases**:
   - Flag carrier disconnect/death
   - Multiple players entering zone
   - Admin intervention scenarios

## Entity Setup Requirements

### Critical Entity Naming Convention

The following entities MUST exist in your world with exact names:

| Entity Name | Purpose | Required When |
|-------------|---------|---------------|
| `FlagSpawn` | Flag spawn location | Always |
| `BluforDropZone1` | BLUFOR drop zone 1 | When BLUFOR enabled |
| `BluforDropZone2` | BLUFOR drop zone 2 | When BLUFOR enabled |
| `OpforDropZone1` | OPFOR drop zone 1 | When OPFOR enabled |
| `OpforDropZone2` | OPFOR drop zone 2 | When OPFOR enabled |
| `IndforDropZone1` | INDFOR drop zone 1 | When INDFOR enabled |
| `IndforDropZone2` | INDFOR drop zone 2 | When INDFOR enabled |

### Entity Placement Best Practices

1. **Flag Spawn Location**:
   - Place centrally between team bases
   - Avoid difficult terrain or inaccessible areas
   - Consider cover and concealment balance

2. **Drop Zones**:
   - Place near team spawn areas but not inside spawn protection
   - Ensure adequate space for the zone radius
   - Consider defensive positions and approaches
   - Maintain balance between teams

3. **Zone Radius Considerations**:
   - Default 25m radius should accommodate most scenarios
   - Adjust based on terrain and base layout
   - Ensure zones don't overlap with spawn protection

## Component Configuration

### Faction Configuration Options

#### Two-Team Setup (BLUFOR vs OPFOR)
```
BLUFOR Enabled: ✓
OPFOR Enabled: ✓
INDFOR Enabled: ✗
```

#### Three-Team Setup (All Factions)
```
BLUFOR Enabled: ✓
OPFOR Enabled: ✓
INDFOR Enabled: ✓
```

#### Custom Faction Setup
Enable only the factions you want participating in your specific scenario.

### Timer Configuration

#### Standard CTF (5-minute capture)
```
Capture Timer: 300
Marker Update Interval: 120
```

#### Fast-Paced CTF (2-minute capture)
```
Capture Timer: 120
Marker Update Interval: 60
```

#### Extended CTF (10-minute capture)
```
Capture Timer: 600
Marker Update Interval: 180
```

### Prefab Path Configuration

Ensure your prefab paths are correctly set:

```
Flag Prefab: "{YOUR_MOD_GUID}Prefabs/Items/CRF_CTF_Flag.et"
Drop Zone Prefab: "{YOUR_MOD_GUID}Prefabs/Triggers/CRF_CTF_DropZone.et"
```

## Prefab Creation

### Creating the CTF Flag Prefab

#### Required Components:
1. **MeshObject** - Visual representation of the flag
2. **CRF_CTFFlagComponent** - Handles flag mechanics
3. **ActionsManagerComponent** - Manages player interactions

#### CRF_CTFFlagComponent Configuration:
- No additional configuration required
- Handles pickup/drop logic automatically

#### ActionsManagerComponent Setup:
1. Add `CRF_CTFPickupFlagAction` to the actions list
2. Configure action properties:
   - **Action Name**: "Pick up Flag"
   - **Interaction Range**: 3.0 meters
   - **Duration**: 0.0 seconds (instant)

#### Visual Recommendations:
- Use a recognizable flag model
- Add appropriate materials and textures
- Consider adding a flagpole or base
- Ensure the model is optimized for performance

### Creating the CTF Drop Zone Prefab

#### Required Components:
1. **CRF_CTFDropZoneComponent** - Handles zone detection and capture logic

#### CRF_CTFDropZoneComponent Configuration:
- **Faction Key**: Set to appropriate faction (BLUFOR/OPFOR/INDFOR)
- **Zone Radius**: 25.0 meters (adjustable based on needs)

#### Visual Recommendations:
- Add area markers or boundary indicators
- Use faction-appropriate colors
- Consider adding decorative elements (flags, banners, etc.)
- Ensure visual elements don't interfere with gameplay

#### Optional Components:
- **StaticModelEntity** for decorative elements
- **DecalEntity** for ground markings
- **ParticleEffectEntity** for atmospheric effects

## Testing and Troubleshooting

### Pre-Launch Checklist

- [ ] All required entities placed with correct names
- [ ] CTF gamemode component added to Game Mode entity
- [ ] Flag and drop zone prefabs created and configured
- [ ] Faction settings match your scenario requirements
- [ ] Prefab paths correctly set in component configuration
- [ ] Timer settings configured appropriately

## Troubleshooting

### 🔧 Common Issues and Solutions

#### Issue: Flag Not Spawning
**Symptoms**: No flag appears in the world at game start
**Causes & Solutions**:
```
❌ FlagSpawn entity missing
   → Create entity named exactly "FlagSpawn" (case-sensitive)

❌ Invalid flag prefab path  
   → Verify prefab path in component configuration
   → Test prefab loads in Resource Browser

❌ Flag prefab missing components
   → Ensure CRF_CTFFlagComponent is present
   → Verify ActionsManagerComponent with CRF_CTFPickupFlagAction

❌ Component initialization failed
   → Check console for error messages
   → Verify CRF framework is loaded as dependency
```

#### Issue: Drop Zones Not Working
**Symptoms**: No capture progress when carrying flag to zones
**Causes & Solutions**:
```
❌ Missing drop zone entities
   → Create required entities: [Faction]DropZone[1-2]
   → Verify exact naming convention (case-sensitive)

❌ Invalid drop zone prefab
   → Ensure CRF_CTFDropZoneComponent is present in prefab
   → Check zone radius setting (default: 25m)

❌ Faction mismatch
   → Verify enabled factions match placed drop zone entities
   → Check player faction matches drop zone faction

❌ Zone positioning issues
   → Ensure zones aren't overlapping with spawn protection
   → Verify zone radius encompasses intended area
```

#### Issue: Players Can't Pick Up Flag
**Symptoms**: No interaction prompt when approaching flag
**Causes & Solutions**:
```
❌ Missing interaction action
   → Add CRF_CTFPickupFlagAction to flag's ActionsManagerComponent
   → Verify action is properly configured

❌ Player state issues
   → Ensure player is alive and conscious
   → Check player faction is enabled in gamemode
   → Verify player is within interaction range (default: 3m)

❌ Flag already carried
   → Only one player can carry flag at a time
   → Check if another player currently has the flag

❌ Permission issues
   → Verify faction permissions allow flag pickup
   → Check for conflicting gamemode restrictions
```

#### Issue: Map Markers Not Updating
**Symptoms**: Flag position not shown or updated on map
**Causes & Solutions**:
```
❌ Markers disabled in settings
   → Set "Hide Map Markers" to false in component

❌ Update interval set incorrectly
   → Verify "Marker Update Interval" > 0
   → Recommended: 120 seconds for balance

❌ Marker system conflicts
   → Check for conflicts with other marker systems
   → Verify CRF marker system is functional

❌ Network synchronization issues
   → Test in dedicated server environment
   → Check client-server replication
```

#### Issue: Capture Timer Problems
**Symptoms**: Timer doesn't start, stops unexpectedly, or behaves incorrectly
**Causes & Solutions**:
```
❌ Player not in correct zone
   → Verify flag carrier is in their faction's drop zone
   → Check zone radius covers the intended area

❌ Timer configuration issues
   → Ensure capture timer > 0 (recommended: 60+ seconds)
   → Verify timer value is reasonable for gameplay

❌ Zone exit behavior
   → Timer pauses/resets when player leaves zone
   → Ensure players stay within zone radius during capture

❌ Faction validation failure
   → Verify player faction matches drop zone faction
   → Check enabled factions configuration
```

### 🚨 Critical Error Messages

#### "Could not find FlagSpawn entity"
```
Cause: Missing or incorrectly named flag spawn entity
Solution: 
1. Create entity in world
2. Name exactly "FlagSpawn" (case-sensitive)
3. Place at desired flag spawn location
```

#### "Failed to spawn flag entity"  
```
Cause: Invalid flag prefab path or corrupted prefab
Solution:
1. Verify prefab exists at specified path
2. Test prefab loads in Resource Browser
3. Check prefab components are properly configured
4. Regenerate prefab if corrupted
```

#### "Drop zone [name] entity not found"
```
Cause: Missing drop zone entity for enabled faction
Solution:
1. Create missing drop zone entities
2. Use exact naming: BluforDropZone1, OpforDropZone2, etc.
3. Verify enabled factions match placed entities
```

#### "CTF component not found on gamemode"
```
Cause: CTF component missing or misconfigured
Solution:
1. Add CRF_CTFGamemodeManager to Game Mode entity
2. Verify component is properly initialized
3. Check for component conflicts
```

### 🧪 Debug Testing Procedure

#### Phase 1: Solo Testing
1. **Launch solo session** with admin privileges
2. **Test basic functionality**:
   - Flag spawns correctly
   - Flag pickup works
   - Drop zones are present and functional
   - Timer mechanics work properly
3. **Check console output** for errors or warnings
4. **Verify faction switching** if testing multiple factions

#### Phase 2: Multiplayer Testing  
1. **Launch with 2+ players** from different factions
2. **Test network synchronization**:
   - Flag pickup/drop replicates to all clients
   - Zone entry/exit syncs properly
   - Timer shows correctly for all players
   - Victory conditions trigger for all clients
3. **Test edge cases**:
   - Player disconnect while carrying flag
   - Multiple players in same zone
   - Admin commands and overrides

#### Phase 3: Performance Testing
1. **Monitor server performance** with multiple players
2. **Check network bandwidth** usage
3. **Validate marker update** frequency impact
4. **Test with maximum expected player count**

### 📋 Debug Console Commands

Enable detailed logging for troubleshooting:
```
// Flag-related events
[CRF_CTFFlagComponent] Debug output for flag mechanics

// Drop zone events  
[CRF_CTFDropZoneComponent] Zone entry/exit and capture events

// Gamemode events
[CRF_CTFGamemodeManager] Overall gamemode state and coordination

// Network events
[RPC] Network replication for CTF events
```

### 🔍 Validation Checklist

Before declaring the implementation complete:

**Entity Validation**:
- [ ] FlagSpawn entity exists with correct name
- [ ] All required drop zone entities present
- [ ] Entity names match exactly (case-sensitive)
- [ ] Entity positions are balanced and accessible

**Component Validation**:
- [ ] CTF component added to Game Mode entity
- [ ] Component settings configured properly
- [ ] Prefab paths are valid and load successfully
- [ ] No conflicting components present

**Gameplay Validation**:
- [ ] Flag spawns and can be picked up
- [ ] Drop zones detect flag carriers correctly
- [ ] Capture timer functions properly
- [ ] Victory conditions trigger correctly
- [ ] Network synchronization works in multiplayer

**Performance Validation**:
- [ ] No significant performance impact
- [ ] Marker updates don't cause lag
- [ ] Zone detection is responsive
- [ ] Memory usage is reasonable

## Performance Optimization

### 🚀 Server Performance

#### Marker Update Optimization
```
High Performance (Low Detail):
- Marker Update Interval: 300+ seconds
- Hide Map Markers: true (for hardcore gameplay)

Balanced Performance (Recommended):
- Marker Update Interval: 120-180 seconds  
- Hide Map Markers: false

High Detail (Performance Cost):
- Marker Update Interval: 60-90 seconds
- Additional UI elements enabled
```

#### Zone Detection Optimization
- **Detection Frequency**: 500ms per zone (not user-configurable)
- **Zone Count Impact**: Each enabled faction adds 2 zones
- **Recommendation**: Only enable factions that will have players

#### Network Traffic Optimization
```
Low Traffic Configuration:
- Fewer enabled factions (2-team vs 3-team)
- Longer marker update intervals
- Minimal UI updates

High Traffic Scenarios:
- 3-team battles with frequent marker updates
- Multiple simultaneous flag carriers (if modded)
- High player count servers (20+ players)
```

### 🎯 Client Performance

#### Rendering Considerations
- **Flag Attachment**: Minimal impact on character rendering
- **Zone Visuals**: Complex zone prefabs can impact FPS in large battles
- **Map Markers**: Frequent updates can stress GUI rendering

#### Optimization Recommendations
```
Zone Prefab Optimization:
- Use low-poly visual elements
- Minimize particle effects
- Avoid complex materials/shaders
- Consider LOD (Level of Detail) for decorative elements

UI Optimization:
- Reduce marker update frequency for large servers
- Disable unnecessary UI elements
- Use efficient notification systems
```

### 📊 Performance Monitoring

#### Key Metrics to Monitor
```
Server Metrics:
- CPU usage during zone detection cycles
- Network bandwidth for CTF replication
- Memory usage growth over time
- Frame time impact during peak gameplay

Client Metrics:
- GUI rendering performance with frequent marker updates
- Character attachment system impact
- Zone visual rendering in large battles
```

#### Performance Testing Protocol
1. **Baseline Testing**: Test without CTF to establish baseline performance
2. **Load Testing**: Test with maximum expected player count
3. **Duration Testing**: Run extended sessions to check for memory leaks
4. **Network Testing**: Monitor bandwidth usage across different scenarios

## Technical Reference

### 📂 File Structure
```
scripts/Game/GameMode/CaptureTheFlag/
├── Components/
│   ├── CRF_CTFGamemodeManager.c        # Main gamemode component
│   ├── CRF_CTFFlagComponent.c          # Flag entity component  
│   ├── CRF_CTFDropZoneComponent.c      # Drop zone component
│   └── CRF_CTFDisplayComponent.c       # UI display component
├── Actions/
│   └── CRF_CTFPickupFlagAction.c       # Flag pickup action
├── UI/
│   ├── CTF_HUD.layout                  # HUD layout file
│   └── CTF_Notifications.c             # Notification system
├── Prefabs/
│   ├── Items/
│   │   └── CRF_CTF_Flag.et            # Default flag prefab
│   └── Triggers/
│       └── CRF_CTF_DropZone.et        # Default drop zone prefab
└── Documentation/
    └── CTF_IMPLEMENTATION_GUIDE.md    # This guide
```

### 🔗 Component Hierarchy
```
Game Mode Entity
└── CRF_CTFGamemodeManager (SCR_BaseGameModeComponent)
    ├── Flag Management
    │   ├── Spawn flag at FlagSpawn entity
    │   ├── Monitor flag state and carrier
    │   └── Handle flag drop events
    ├── Drop Zone Coordination
    │   ├── Spawn zones at designated entities
    │   ├── Monitor zone entry/exit events
    │   └── Manage capture timers
    ├── Victory Condition Management
    │   ├── Monitor capture progress
    │   ├── Declare victory on timer completion
    │   └── Reset gamemode for next round
    └── Network Synchronization
        ├── Replicate flag state to clients
        ├── Sync zone entry/exit events
        └── Broadcast victory conditions

Flag Entity (Spawned)
└── CRF_CTFFlagComponent (ScriptComponent)
    ├── Pickup/Drop Mechanics
    │   ├── Handle player interaction
    │   ├── Attach to carrier character
    │   └── Drop on death/disconnect
    ├── Carrier Tracking
    │   ├── Monitor carrier position
    │   ├── Update map markers
    │   └── Track zone entry/exit
    └── Visual Management
        ├── Flag attachment rendering
        ├── Carrier movement effects
        └── Drop animation handling

Drop Zone Entity (Spawned)
└── CRF_CTFDropZoneComponent (ScriptComponent)
    ├── Area Monitoring
    │   ├── Detect players in zone radius
    │   ├── Validate faction membership
    │   └── Check for flag carrier
    ├── Capture Timer Management
    │   ├── Start timer on flag carrier entry
    │   ├── Pause timer on carrier exit
    │   └── Complete capture on timer finish
    └── Zone Visualization
        ├── Optional visual indicators
        ├── Faction-specific styling
        └── Capture progress display
```

### 🌐 Network Architecture

#### RPC (Remote Procedure Call) System
```
Client → Server RPCs:
- Flag pickup requests
- Drop zone entry notifications
- Player action validation

Server → Client RPCs:
- Flag state updates
- Capture timer synchronization
- Victory announcements
- Map marker updates

Broadcast RPCs:
- Flag position updates
- Zone capture progress
- Game state changes
```

#### State Synchronization
```
Flag State:
- Current carrier (player ID)
- Flag position (world coordinates)
- Pickup/drop timestamps
- Last known zone

Zone State:
- Active capture timers
- Players in zone
- Faction ownership
- Capture progress percentage

Game State:
- Enabled factions
- Victory conditions
- Round timer
- Match statistics
```

### 🔧 Integration Points

#### CRF Framework Integration
```
Manager Systems:
- CRF_PlayerManager: Player tracking and faction validation
- CRF_NotificationManager: Player notifications and messages
- CRF_MarkerManager: Map marker system integration
- CRF_VictoryManager: Victory condition handling

Event Systems:
- Player death/disconnect events
- Faction change events
- Admin intervention events
- Round start/end events

UI Systems:
- HUD integration for capture progress
- Map integration for flag markers
- Notification system for events
- Admin panel integration
```

#### Third-Party Compatibility
```
Respawn Systems:
- Compatible with custom respawn managers
- Handles flag drop on player death
- Respects spawn protection zones

Admin Tools:
- Admin commands for flag manipulation
- Zone management tools
- Timer override capabilities
- Game state inspection tools

Statistics Systems:
- Flag pickup/drop tracking
- Capture attempt logging
- Victory condition statistics
- Player performance metrics
```

### 📋 Configuration Reference

#### Complete Component Configuration
```
CRF_CTFGamemodeManager Settings:

Faction Settings:
├── BLUFOR Enabled: boolean (default: true)
├── OPFOR Enabled: boolean (default: true)
└── INDFOR Enabled: boolean (default: false)

Timing Settings:
├── Capture Timer: integer (default: 300, range: 60-1800)
├── Marker Update Interval: integer (default: 120, range: 0-600)
└── Reset Delay: integer (default: 10, range: 5-60)

Prefab Settings:
├── Flag Prefab: ResourceName (path to flag prefab)
├── Drop Zone Prefab: ResourceName (path to zone prefab)
└── Effect Prefabs: array<ResourceName> (optional effects)

Gameplay Settings:
├── Hide Map Markers: boolean (default: false)
├── Allow Flag Drop: boolean (default: true)
├── Require Zone Hold: boolean (default: true)
└── Multi-Carrier Mode: boolean (default: false) [Future]

Advanced Settings:
├── Zone Detection Radius: float (default: 25.0, range: 5.0-100.0)
├── Pickup Range: float (default: 3.0, range: 1.0-10.0)
├── Marker Precision: enum (High/Medium/Low)
└── Debug Mode: boolean (default: false)
```

---

## Support and Community

### 📞 Getting Help

**Official Channels**:
- CRF Framework Documentation: [Link to docs]
- Community Discord: [Discord invite]
- GitHub Issues: [Repository URL]
- Steam Workshop: [Workshop page]

**Common Questions**:
1. **Compatibility**: Compatible with most CRF framework missions
2. **Updates**: Follows CRF framework update schedule
3. **Customization**: Fully customizable through prefabs and configuration
4. **Performance**: Optimized for servers up to 64 players

### 🤝 Contributing

**Bug Reports**:
- Use GitHub issues with detailed reproduction steps
- Include log files and configuration details
- Test in minimal reproduction environment

**Feature Requests**:
- Community discussion before implementation
- Consider performance and compatibility impact
- Maintain CRF framework design principles

**Code Contributions**:
- Follow CRF coding standards
- Include comprehensive testing
- Update documentation for any changes

---

**Need Help?** Check the CRF framework documentation, join the community Discord, or create a GitHub issue with detailed information about your setup and the specific problem you're encountering.