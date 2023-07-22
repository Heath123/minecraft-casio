from nbt.nbt import NBTFile

nbtfile = NBTFile("chunk.nbt", "rb")

size = [x.value for x in nbtfile["size"]]
# print("Size:", size)

# for x in nbtfile["size"][0]:
#   for y in nbtfile["size"][1]:
#     for z in nbtfile["size"][2]:
#       	block = nbtfile["blocks"][x * size[1] * this.size[2] + pos[1] * this.size[2] + pos[2]]

# print(nbtfile["palette"][0])

for block in nbtfile["blocks"]:
  pos = [x.value for x in block["pos"]]
  if pos[0] < 32 and pos[2] < 32:
    state = block["state"].value
    name = nbtfile["palette"][state]["Name"].value
    # print(name)
    blockID = 2
    if name == "minecraft:air":
      blockID = 0
    elif name == "minecraft:dirt" or name == "minecraft:grass_block":
      blockID = 1
    elif name == "minecraft:oak_leaves":
      blockID = 3
    elif name == "minecraft:oak_log":
      blockID = 4
    if blockID != 0:
      print(f"chunk->setBlock({pos[0]}, {pos[1]}, {pos[2]}, {blockID});")
