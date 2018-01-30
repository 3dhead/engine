# Shader

# mesh.frag

there is an error in the fog computation - right now everything is in u_fogcolor because fogval == 1
this is due to the fact that fogdistance is a very very high value because gl_FragCoord.w is 0

# Data handling

The current approach with data dir must be extended/redone someone. There are way too many files installed per artifact atm. Also there should be CPack support to generate debian packages.

# Persistence
## Checks

`ALTER TABLE products ADD CHECK (name <> '');`

or even better - do it in code.

## Prepared statements

Better support for prepared statements

## Joins

Add support (autogenerate) for joins for the foreign keys in the models

## Savable

Support for deleting models

## Enums

Support enums?

`CREATE TYPE mood AS ENUM ('sad', 'ok', 'happy');`

## Databasetool

* Support `(x, y, z) primarykey` (like we do for unique keys)
* Lock table metainfo whenever we try to update the database structure

# Shadertool

Validate that each `$out` of the vertex shader has a `$in` in the fragment shader and vice versa.

# VoxEdit

Extract meshes in max 32x32x32 boundaries - `RawVolumeRenderer` maybe? This allows us to only reextract the chunk that was modified, not always the whole volume.

Voxelizer via assimp

Extrude

Scale/move/rotate selections - not only the cursor or whole model

LSystem parameter window needs the voxel options and should show information about the used alphabet

Move space colonization tree generator into the tree submenu and take the ToolWindow parameters into account

# EventMgr

## Proper event-end-while-application-is-not-running handling

If an event was stopped, this must be persisted in the event table - skip those in the event provider. if the endtime has passed, but that flag is not set, just load the event as usual and stop it in the next frame this is needed if the server had a downtime while the event would have ended. In such a case, no loot would get hand out to the players. To work around this, we let the event just restore all its states and then stop properly.

## Rename EventProvider to EventSelector

The EventProvider should read the event lua scripts - the `EventSelector` should read the event run configuration (start- and end dates) from the database

# LootMgr

Implement `LootMgr` that is able to hand out or change items via the stock system. Implement lua loot definition 'parsing'.

# UserStockMgr

Implement saving and loading of `Stock` and `Inventory` data.

# Stock

Think about the design again - `Item` shouldn't be a `shared_ptr`, just use the type in the map. That would also make an atomic counter useless, because we aren't operating on a shared data anymore. The `Stock` instance would be the owner and only instance that is able to modify the internal data.

Container move operations with filters - if might e.g. cost currency to move from one container to another.

# World

Implement island style maps. Each `Map` instance gets its own ai `Zone`. There should be portals on the islands to get to other islands. Provide an overview map of where the islands are.
(Theoretically we could scale/cluster per `Map` and each jump could include a server switch)
Maybe also allow to sail to a another island...?

These islands should not only be created by noise - just supported by noise to vary. But they should still be hand crafted to make them more interesting.

The `World` class manages the `Map` classes and should have a threadpool to update the maps in. Use libuv timers and forward the updating into the threads.

The `Map` should have a lua tick - which is e.g. able to spawn new npcs or let stuff happen on the map. It needs access to all the users, all the npcs and must be be called on events like user-add/remove-from-map and npc-add/remove-from-map.

## SpawnMgr

Configure entity types and amounts via lua map script

## AttackMgr

The attack mananger should get updated in the map tick and should maintain a list of attackers and victims.

## Map editor

There should be a map editor to place Point-of-Interests (see `PoiProvider`) and entities with attributes. E.g. you should be able to place markers to modifiy the island shape and put attributes into these. The map editor should just allow to modifiy these markers and place entities. Placing stuff on a per-voxel level should not be supported. The real map building should still be implemented via procgen.

# Statsd support

Extend statsd support with more events

# SignUp

Implement signup with email verification, Lost password and co - also see password related point in the persistence section.
Maybe use https://github.com/est31/csrp-gmp

# UI

Get rid of nativefiledialog - e.g. for nuklear use their file dialog implementation.

# Compute module (opencl)

- Support adding `#pragma unroll` automatically?
- Support opencl data types - `image2d_t`, `sampler_t`
- `clEnqueueWriteImage` and friends
