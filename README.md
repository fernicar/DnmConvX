# DnmConvX

**DnmConvX** is a command-line utility designed to convert YSFlight Dynamodel (`.dnm`) files into the DirectX (`.x`) file format. This allows YSFlight models to be used in applications and game engines that support the DirectX mesh format.

The tool also supports an optional INI configuration file for more advanced control over the conversion process, including mesh and frame blacklisting, material handling, and animation pose adjustments.

## Features

*   Converts `.dnm` model geometry, materials, and basic structure.
*   Supports animation data translation from `.dnm`'s format to DirectX animation sets.
*   Allows fine-tuning of the conversion via an `.ini` configuration file.
*   Provides options for:
    *   Nested material and mesh definitions.
    *   Merging empty animation frames.
    *   Blacklisting specific meshes or frames from the output.
    *   Designating materials as double-sided.
    *   Inverting face normals by index or material.
    *   Custom animation pose configurations.

## Command-Line Usage

The basic syntax for using DnmConvX is:

```sh
DnmConvX.exe <input_dnm_file_path> [output_x_file_path]
```

*   `<input_dnm_file_path>`: **Required.** Specifies the path to the input `.dnm` file.
*   `[output_x_file_path]`: **Optional.** Specifies the desired path for the output `.x` file. If omitted, the `.x` file will be created in the same directory as the input `.dnm` file, with the same base name (e.g., `airplane.dnm` will produce `airplane.x`).

**Examples:**

1.  Convert `airplane.dnm` and save as `airplane.x` in the same directory:
    ```sh
    DnmConvX.exe airplane.dnm
    ```

2.  Convert `my_model.dnm` and save to a specific path `C:\output\my_model_converted.x`:
    ```sh
    DnmConvX.exe my_model.dnm C:\output\my_model_converted.x
    ```

3.  Display help information:
    ```sh
    DnmConvX.exe /H
    ```

## Configuration File (`.ini`)

DnmConvX automatically looks for and loads configuration settings from `.ini` files in two locations (in this order):

1.  **Next to the executable:** An `.ini` file with the same base name as the executable (e.g., `DnmConvX.ini`) located in the same directory as `DnmConvX.exe`.
2.  **Next to the input `.dnm` file:** An `.ini` file with the same base name as the input `.dnm` file (e.g., if converting `airplane.dnm`, it looks for `airplane.ini`) located in the same directory as the `.dnm` file.

Settings from both files are merged if both exist, with settings from the `.dnm`-specific `.ini` file potentially overriding those from the executable-specific `.ini`.

### INI File Format and Options

The `.ini` file uses standard INI formatting with sections and key-value pairs. Comments start with `#`.

**Supported Sections and Options:**

*   **`[Config]`**: General conversion settings.
    *   `UseNestedMaterial`: Set to `true` to enable nested material definitions in the output .x file. Default: `false`.
    *   `UseNestedMesh`: Set to `true` to enable nested mesh definitions in the output .x file. Default: `false`.
    *   `MergeEmptyFrames`: Set to `true` to merge animation frames that have no associated mesh and a single child frame, potentially simplifying the animation hierarchy. Default: `false`.

*   **`[BlacklistMesh]`**: Lists meshes to exclude from the conversion.
    *   Each line is a mesh name (e.g., `INTERNAL_DETAIL_MESH`).

*   **`[BlacklistFrame]`**: Lists animation frames (and their associated data/children) to exclude.
    *   Each line is a frame name (e.g., `LANDING_GEAR_DEBUG_FRAME`).

*   **`[DoubleSideMesh]`**: Lists materials that should be marked as double-sided in the output .x file.
    *   Each line is a material name. (Note: The actual application of double-sided rendering depends on the engine loading the .x file).

*   **`[InvertFaceByIdx]`**: Specifies meshes and face indices within those meshes where face normals should be inverted.
    *   Format: `MeshName FaceIndex` (e.g., `FUSELAGE 101`).

*   **`[InvertFaceByMaterial]`**: Specifies meshes and material names; faces using these materials within the specified meshes will have their normals inverted.
    *   Format: `MeshName MaterialName` (e.g., `WING_LEFT TransparentCanopyMaterial`).

*   **`[PoseF]`, `[PoseG]`, `[PoseB]`**: Used for custom animation pose configurations, typically for models with multiple transformation states (e.g., Fighter, Gerwalk, Battroid modes in variable fighter aircraft).
    *   The first line in each section, starting with `k`, defines a set of keyframe numbers (e.g., `k 0 10 20`).
    *   Subsequent lines define `CLA STA` pairs (Animation Class and State Index from the DNM) that map to these keyframes. For each `CLA STA` pair, the converter will generate animation keys at the previously defined keyframe numbers, using the pose data (position, orientation) from that specific `STA` for the given `CLA`.
    *   Example for `[PoseF]`:
        ```ini
        k 0 100 200 # Defines keyframes 0, 100, 200 for this pose
        1 0         # Animation Class 1, State 0 will be mapped to keyframes 0, 100, 200
        2 5         # Animation Class 2, State 5 will be mapped to keyframes 0, 100, 200
        ```

## Conversion Process Overview

The following diagrams illustrate the main data processing flows within DnmConvX.

### 1. DNM File Parsing (Conceptual)

This diagram shows the conceptual flow of how a `.dnm` file is parsed into its constituent parts like meshes, frames, and animation data. Due to the complexity of the original parsing logic, this is a high-level representation.

```mermaid
graph TD
    A[Start DNM Parse] --> B{Read Header};
    B -- DYNAMODEL Version 1 --> C{Iterate Lines};
    B -- Other --> Z[Error: Invalid DNM];
    C -- PCK (Surface Pack Start) --> D[Store Previous Mesh, Start New Mesh];
    D --> C;
    C -- V (Vertex/Face Index) --> E{Face Mode?};
    E -- Yes --> F[Parse Face Indices];
    F --> C;
    E -- No --> G[Parse Vertex Coords];
    G --> C;
    C -- N (Normal) --> H[Parse Normal Coords];
    H --> C;
    C -- C (Color) --> I[Parse Color];
    I --> C;
    C -- F (Face Start) --> J[Enter Face Mode];
    J --> C;
    C -- E (End of Face/Surface) --> K{End of Face?};
    K -- Yes --> L[Finalize Face, Update Material];
    L --> C;
    K -- No (End of Surface) --> M[Finalize Mesh];
    M --> C;
    C -- B (Bright Face) --> N[Flag Material as Bright/Emissive];
    N --> C;
    C -- SRF (Frame Data Start, after Meshes) --> O[Start Frame Data Parsing];
    O --> P{Iterate Frame Lines};
    P -- SRF (Frame Name) --> Q[New Frame + AnimKey];
    Q --> P;
    P -- FIL (Mesh File Link) --> R[Link Mesh to Frame];
    R --> P;
    P -- CLA (Animation Class) --> S[Set AnimKey Class];
    S --> P;
    P -- STA (Static Pose / Anim State) --> T[Add Pose/State to AnimKey];
    T --> P;
    P -- POS (Position/Orientation) --> U[Set Frame Base Pose];
    U --> P;
    P -- CNT (Center) --> V[Set Mesh Center Offset for Frame];
    V --> P;
    P -- CLD (Child Frame) --> W[Add Child Frame Link];
    W --> P;
    P -- END (End of Frame) --> X[Finalize Frame, Store Frame & AnimKey];
    X --> P;
    P -- End of File --> Y[End DNM Parse];
```

### 2. INI File Processing

This diagram outlines how DnmConvX processes `.ini` configuration files to customize the conversion.

```mermaid
graph TD
    A[Start INI Processing] --> B{"Locate INI (Executable Path)"};
    B -- Found --> C[Parse INI File 1];
    B -- Not Found --> D{"Locate INI (DNM Path)"};
    C --> D;
    D -- Found --> E[Parse INI File 2, Merge/Override Config];
    D -- Not Found --> F[Use Default Config];
    E --> G[Apply Configurations];
    F --> G;
    G --> H{Iterate Config Sections};
    H -- [Config] --> I[Set Global Flags: NestedMaterial, NestedMesh, MergeEmpty];
    I --> H;
    H -- [BlacklistMesh] --> J[Store Mesh Blacklist];
    J --> H;
    H -- [BlacklistFrame] --> K[Store Frame Blacklist];
    K --> H;
    H -- [DoubleSideMesh] --> L[Store DoubleSide Material List];
    L --> H;
    H -- [InvertFaceByIdx] --> M[Store Faces to Invert by Index];
    M --> H;
    H -- [InvertFaceByMaterial] --> N[Store Faces to Invert by Material];
    N --> H;
    H -- [PoseF/G/B] --> O[Parse Pose Keyframes & CLA/STA Mappings];
    O --> H;
    H -- End of INI --> P[INI Processing Complete];
```

### 3. Overall Conversion Flow

This diagram provides a high-level view of the entire conversion process, from command-line arguments to the final `.x` file output.

```mermaid
graph TD
    A[Start DnmConvX] --> B[Parse Command Line Args];
    B --> C[Input DNM File];
    C -- Success --> D[Parse DNM Data];
    C -- Failure --> X[Error: DNM Read];
    D --> E["Input INI File(s)"];
    E -- Optional --> F[Parse INI Data & Apply Configs];
    F --> G[Finalize Data Processing];
    G --> H[Apply Blacklists];
    H --> I[Apply Face Inversions];
    I --> J[Process Animations & Poses based on INI];
    J --> K[Construct DirectX File Structure];
    K --> L[Output .X File];
    L -- Success --> M[Conversion Complete];
    L -- Failure --> Y[Error: X File Write];
    F -- No INI or INI Read Error --> G;
```
