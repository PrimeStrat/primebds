# PrimeBDS

An all-in-one essentials plugin for the Endstone mod loader.

## Code Style

### C++ (clang-format + clang-tidy)
- Based on K&R/1TBS styling
- Naming conventions (from .clang-tidy):
    - Classes/Structs/Enums: CamelCase
    - Methods: camelBack
    - Private/protected members: lower_case_ (trailing underscore)
    - Local variables/parameters: lower_case
    - Macros: UPPER_CASE
- Document code above each function
- Important configuration per command should be registered near the top
- Avoid inline comments for obvious actions and keep to simple 1-line comments for functions unless the function is too long or too complex

## Mod Loader
- GitHub: https://github.com/EndstoneMC/endstone

## Protocol
- https://github.com/SculkCatalystMC/Protocol
- https://prismarinejs.github.io/minecraft-data/

## Minecraft Bedrock Edge Cases
- Containers (even if it's a ghost container) MUST be within 5 blocks of the player for them to have the container opened
