# Class "Room"

## Functions

### Can·Spawn·Obstacle·At·Position () {: aria-label='Functions' }
#### boolean CanSpawnObstacleAtPosition ( int GridIndex, boolean Force ) {: .copyable aria-label='Functions' }

___

### Get·Camera () {: aria-label='Functions' }
#### [Camera](Camera.md) GetCamera ( ) {: .copyable aria-label='Functions' }
Returns a [Camera](Camera.md) object.

___

### Get·Shop·Item·Price () {: aria-label='Functions' }
#### int GetShopItemPrice ( int EntityVariant, int EntitySubType, int ShopItemID ) {: .copyable aria-label='Functions' }
Returns the price of the item.

___
### Remove·Grid·Entity·Immediate () {: aria-label='Functions' }
#### void RemoveGridEntityImmediate ( int GridIndex, int PathTrail, boolean KeepDecoration ) {: .copyable aria-label='Functions' }
*Immediately* removes the GridEntity at the given index. This means grids can be properly removed and immediately replaced, *without* the need to call Room:Update.

___
