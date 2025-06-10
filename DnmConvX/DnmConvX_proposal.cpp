// DnmConvX_proposal.cpp
// Proposed revamped implementation for DnmConvX.cpp

#include "DnmConvX.h"
#include <filesystem> // For std::filesystem::path
#include <string>     // For std::string, std::getline, substr, etc.
#include <vector>     // For std::vector
#include <sstream>    // For std::istringstream, std::stringstream
#include <fstream>    // For std::ifstream, std::ofstream
#include <iostream>   // For std::cout, std::cerr, std::endl
#include <iomanip>    // For std::setw, std::right
#include <map>        // For std::map
#include <algorithm>  // For std::find_if_not, and general algorithms
#include <cctype>     // For ::isspace

// Anonymous namespace for helper functions and constants internal to this translation unit
namespace {

/**
 * @brief Extracts the base name of a file from a string path, removing the extension.
 * @param filePath The full path or filename.
 * @return The base name of the file.
 */
std::string getFileBaseName(const std::string& filePath) {
    if (filePath.empty()) return "";
    std::filesystem::path p(filePath);
    return p.stem().string();
}

/**
 * @brief Trims leading and trailing whitespace from a string.
 * @param s The string to trim.
 * @return A new string with whitespace trimmed.
 */
std::string trimString(const std::string& s) {
    auto start = std::find_if_not(s.begin(), s.end(), [](unsigned char c){ return std::isspace(c); });
    auto end = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char c){ return std::isspace(c); }).base();
    return (start < end ? std::string(start, end) : std::string());
}

/**
 * @brief Removes quotation marks from the beginning and end of a string, if present.
 * @param s The string to unquote.
 * @return The unquoted string.
 */
std::string unquoteString(const std::string& s) {
    if (s.length() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.length() - 2);
    }
    return s;
}

// Constants for parsing
constexpr float DNM_SCALE_FACTOR = 0.01f;
constexpr float DNM_THRESHOLD_MAX_POS = 20.0f; // Max value for certain position coordinates before scaling

} // anonymous namespace

/**
 * @brief Default constructor for CDnmConvX.
 * Initializes member collections and flags to their default states.
 * The actual CDnmConvX object passed to helper structs like SMapCollMsh is `*this`.
 */
CDnmConvX::CDnmConvX(void)
    : mts(*this), omts(*this), mhs(*this), omhs(*this), frs(*this), aks(*this),
      nstmt(false), nstmh(false), mnm(false), inFilePath("") {
    // All members are initialized in the initializer list.
}

/**
 * @brief Parses the header of a DNM or SRF file stream.
 * @param iss Input string stream containing the DNM/SRF data.
 * @param line_idx Reference to the current line index, will be incremented.
 * @param onDnm Output flag, set if DNM header is found.
 * @param onSurf Output flag, set if SRF header is found.
 * @return True if a valid header was parsed, false otherwise.
 */
bool CDnmConvX::parseDnmSrfHeader(std::istringstream& iss, std::uint32_t& line_idx, E_flib& onDnm, E_flib& onSurf) {
    std::string line_buf;
    ++line_idx;
    if (!std::getline(iss, line_buf)) return false;

    line_buf = trimString(line_buf);

    if (line_buf == "DYNAMODEL") {
        onDnm = E_flib::YES;
        ++line_idx;
        if (!std::getline(iss, line_buf)) {
            std::cerr << "Error: Malformed DNM file. Expected version after DYNAMODEL header on line " << line_idx << "." << std::endl;
            return false;
        }
        line_buf = trimString(line_buf);
        if (line_buf.length() < 8 || line_buf.substr(0,8)!="VERSION1") {
            std::cerr << "Warning: Unsupported DNM version or malformed version string: " << line_buf << " on line " << line_idx << "." << std::endl;
            return false;
        }
    } else if (line_buf == "SURF") {
        onSurf = E_flib::YES;
    } else {
        std::cerr << "Error: File is not a recognized DNM or SRF file. Header: '" << line_buf << "' on line " << line_idx << "." << std::endl;
        return false;
    }
    return true;
}

/**
 * @brief Parses a vertex line ('V') from the DNM stream.
 * @param line_stream Stream for the current line.
 * @param currentMesh The mesh to add vertex/face data to.
 * @param onFace Flag indicating if currently parsing face indices.
 */
void CDnmConvX::parseVertexLine(std::istringstream& line_stream, SMesh& currentMesh, E_flib onFace) {
    if (onFace == E_flib::YES) {
        SFaceIdx face_indices;
        std::uint16_t temp_int;
        while (line_stream >> temp_int) {
            face_indices.vfi.push_back(temp_int);
        }
        if (!face_indices.vfi.empty()) {
            currentMesh.fcs.push_back(face_indices);
            currentMesh.normal.fcs.push_back(face_indices);
        }
    } else {
        SVertex current_vertex;
        char round_char = 0;
        line_stream >> current_vertex.x >> current_vertex.y >> current_vertex.z;
        if (line_stream >> round_char) {
             current_vertex.r = (round_char == 'R');
        } else {
            current_vertex.r = false;
        }
        currentMesh.vts.push_back(current_vertex * DNM_SCALE_FACTOR);
    }
}

/**
 * @brief Parses a normal line ('N') from the DNM stream.
 * @param line_stream Stream for the current line.
 * @param currentMesh The mesh to add normal data to.
 */
void CDnmConvX::parseNormalLine(std::istringstream& line_stream, SMesh& currentMesh) {
    std::string garbage_str;
    SVertex normal_vertex;

    for (int i = 0; i < 3; ++i) {
        char peek_char = line_stream.peek();
        while(std::isspace(peek_char) && peek_char != EOF) { line_stream.get(peek_char); peek_char = line_stream.peek(); }
        if (peek_char == EOF || std::isdigit(peek_char) || peek_char == '-' || peek_char == '.') break;
        if (!(line_stream >> garbage_str)) break;
    }

    line_stream >> normal_vertex.x >> normal_vertex.y >> normal_vertex.z;
    currentMesh.normal.vts.push_back(normal_vertex);
}

/**
 * @brief Parses a color line ('C') from the DNM stream.
 * @param line_stream Stream for the current line.
 * @param color15bit Output for 15-bit color structure.
 * @param color24bit Output for 24-bit color structure.
 */
void CDnmConvX::parseColorLine(std::istringstream& line_stream, UColor15Bit& color15bit, UColor24Bit& color24bit) {
    std::uint16_t temp_g, temp_b;

    line_stream >> color15bit.u;

    if (line_stream >> temp_g) {
        if (line_stream >> temp_b) {
            color24bit.r = static_cast<std::uint8_t>(color15bit.u);
            color24bit.g = static_cast<std::uint8_t>(temp_g);
            color24bit.b = static_cast<std::uint8_t>(temp_b);
            color24bit.a = 255;
        } else {
             color24bit = color15bit;
        }
    } else {
        color24bit = color15bit;
    }
}

/**
 * @brief Handles material property updates at the end of a face definition.
 * @param currentMesh The mesh being built.
 * @param current_material_template The current material properties being defined/updated.
 * @param color24bit The color of the just-completed face.
 * @param isBright Flag indicating if the face/material is emissive.
 * @param current_mtname Output parameter for the generated/resolved material name.
 */
void CDnmConvX::handleEndOfFace(SMesh& currentMesh, SMaterial& current_material_template, const UColor24Bit& color24bit, E_flib& isBright, std::string& current_mtname) {
    current_mtname = color24bit.operator std::string();
    SMaterial face_material = current_material_template;
    face_material.d = color24bit;

    if (isBright == E_flib::YES) {
        face_material.e = face_material.d;
        if (!current_mtname.empty()) current_mtname[0] = 'G';
        isBright = E_flib::NOT;
    } else {
        face_material.e = SColor3Float{4 / 255.f, 4 / 255.f, 4 / 255.f};
    }
    face_material.name = current_mtname;

    if (!currentMesh.mlist.mtMap.count(current_mtname)) {
        mts << face_material;
        if (!nstmt) {
            omts << face_material;
        }
        currentMesh.mlist.mtMap[current_mtname] = face_material;
    }
    currentMesh.mlist.mtIdx.push_back(current_mtname);
}

/**
 * @brief Main parsing operator for DNM data from an input string stream.
 * This function reads the stream line by line, interpreting DNM commands
 * to populate mesh, material, frame, and animation data structures.
 * @param iss The input string stream containing DNM data.
 * @return A reference to the CDnmConvX object itself.
 */
CDnmConvX& CDnmConvX::operator<<(std::istringstream& iss) {
    E_flib parse_mode_dnm = E_flib::NOT;
    E_flib parse_mode_surf = E_flib::NOT;
    E_flib parse_mode_pck = E_flib::NOT;
    E_flib parse_mode_srf_hierarchy = E_flib::NOT;

    std::uint32_t current_line_idx = 0;
    std::string line_buf;

    if (!parseDnmSrfHeader(iss, current_line_idx, parse_mode_dnm, parse_mode_surf)) {
        return *this;
    }

    SMaterial current_material_template;
    current_material_template.name = "DefaultDnmMaterial";
    current_material_template.g = 50.0f * 1.28f;
    current_material_template.s = {50 / 255.f, 50 / 255.f, 50 / 255.f};
    if(!mts.mtMap.count(current_material_template.name)) mts << current_material_template;
    if(!nstmt && !omts.mtMap.count(current_material_template.name)) omts << current_material_template;

    SMesh current_processing_mesh(*this);
    E_flib onFace = E_flib::NOT;
    E_flib isBright = E_flib::NOT;
    UColor15Bit color15bit_face;
    UColor24Bit color24bit_face;
    std::string current_mtname_face;
    std::string srf_frame_name_buffer; // Buffer for SRF frame name if mesh loop breaks on SRF

    while (++current_line_idx, std::getline(iss, line_buf)) {
        line_buf = trimString(line_buf);
        if (line_buf.empty() || line_buf[0] == ';') continue;

        std::istringstream line_stream(line_buf);
        char command_char = line_buf[0];
        std::string keyword;

        // For single-char commands, consume the command char from line_stream
        // For multi-char commands (SRF, PCK), they will consume from line_stream themselves
        if (std::string("VNCFEBS").find(command_char) != std::string::npos) {
             char consumed_cmd_char;
             line_stream >> consumed_cmd_char;
        }

        switch (command_char) {
            case 'V':
                parseVertexLine(line_stream, current_processing_mesh, onFace);
                break;
            case 'N':
                parseNormalLine(line_stream, current_processing_mesh);
                break;
            case 'C':
                parseColorLine(line_stream, color15bit_face, color24bit_face);
                break;
            case 'F':
                onFace = E_flib::YES;
                break;
            case 'E':
                if (onFace == E_flib::YES) {
                    handleEndOfFace(current_processing_mesh, current_material_template, color24bit_face, isBright, current_mtname_face);
                    onFace = E_flib::NOT;
                } else if (parse_mode_surf == E_flib::YES) {
                    parse_mode_surf = E_flib::NOT;
                }
                break;
            case 'S':
                keyword = line_buf;
                if (keyword == "SURF") {
                    parse_mode_surf = E_flib::YES;
                } else if (keyword.rfind("SRF", 0) == 0) {
                    if (!current_processing_mesh.name.empty() || !current_processing_mesh.vts.empty()) {
                         mhs << current_processing_mesh;
                         current_processing_mesh.clear();
                    }
                    parse_mode_srf_hierarchy = E_flib::YES;

                    std::istringstream srf_line_stream(line_buf); // Use a new stream for SRF line
                    std::string srf_keyword_token;
                    srf_line_stream >> srf_keyword_token >> srf_frame_name_buffer; // Store frame name
                    goto end_mesh_parsing;
                }
                break;
            case 'P':
                keyword = line_buf.substr(0,3);
                if (keyword == "PCK") {
                    parse_mode_pck = E_flib::YES;
                    if (!current_processing_mesh.name.empty() || !current_processing_mesh.vts.empty()) {
                        mhs << current_processing_mesh;
                    }
                    current_processing_mesh.clear();
                    std::string pck_keyword_token, pck_filename_ext;
                    // Re-use line_stream as it was not advanced for 'P' yet by the switch
                    std::istringstream p_line_stream(line_buf);
                    p_line_stream >> pck_keyword_token >> pck_filename_ext;
                    current_processing_mesh.name = getFileBaseName(pck_filename_ext);
                    parse_mode_surf = E_flib::YES;
                }
                break;
            case 'B':
                isBright = E_flib::YES;
                break;
            default:
                break;
        }
    }
end_mesh_parsing:;

    if (!current_processing_mesh.name.empty() || !current_processing_mesh.vts.empty()) {
        mhs << current_processing_mesh;
    }

    if (parse_mode_srf_hierarchy == E_flib::YES || !iss.eof()) {
         // Pass the buffered SRF line (if any) or continue with iss
         std::istringstream frame_data_iss( (parse_mode_srf_hierarchy == E_flib::YES && !srf_frame_name_buffer.empty()) ? ("SRF " + srf_frame_name_buffer + "\n" + iss.str().substr(static_cast<size_t>(iss.tellg())) ) : iss.str().substr(static_cast<size_t>(iss.tellg())) );

         // If we jumped here due to SRF, current_line_idx was already incremented for that SRF line.
         // If we are just continuing, it's fine.
         // The parseFrameAndAnimationData needs to handle the first line correctly if it's the buffered SRF line.
         // A simpler way might be to just let parseFrameAndAnimationData take the main `iss` and `line_idx`
         // and it tries to read the first line. If that first line was the SRF line, `line_idx` would be correct.

         // Resetting stream position if we used seekg before
         if (parse_mode_srf_hierarchy == E_flib::YES && !srf_frame_name_buffer.empty()) {
             // The line_idx is for the SRF line. parseFrameAndAnimationData will read it.
         } else if (iss.eof()){
            // no more data for frame parsing
         }

         parseFrameAndAnimationData(iss, current_line_idx);
    }

    return *this;
}


/**
 * @brief Parses frame and animation data from the DNM stream.
 * This typically follows the mesh data and defines the scene hierarchy and animations.
 * @param iss Input string stream, positioned at the start of frame/animation data.
 * @param line_idx Reference to the current line index, will be incremented (used for error reporting).
 */
void CDnmConvX::parseFrameAndAnimationData(std::istringstream& iss, std::uint32_t& line_idx) {
    SFrame current_frame(*this);
    E_flib onSrfSection = E_flib::NOT;
    std::string line_buf;

    // This loop handles the case where the first line for frame parsing might have been
    // the SRF line that broke the mesh parsing loop.
    // If iss is already at EOF or failed state from mesh parsing, this loop won't run.
    bool first_line_processed = false;

    while (first_line_processed ? std::getline(iss, line_buf) : (std::getline(iss, line_buf) || true) ) { // Modified loop condition
        if (!first_line_processed) {
            first_line_processed = true; // Ensure current line_buf (potentially SRF line) is processed
            // If line_buf is empty here and it was the first attempt, it means iss was already at EOF
            if (line_buf.empty() && iss.eof()) break;
        } else { // For subsequent lines
             ++line_idx;
        }

        line_buf = trimString(line_buf);
        if (line_buf.empty() || line_buf[0] == ';') {
            if (iss.eof()) break;
            continue;
        }

        std::istringstream line_stream(line_buf);
        std::string keyword;
        line_stream >> keyword;

        if (keyword == "SRF") {
            if (onSrfSection == E_flib::YES && !current_frame.name.empty()){
                 processAndStoreCurrentFrame(current_frame);
                 current_frame.clear();
            }
            onSrfSection = E_flib::YES;
            std::string srf_name_quoted;
            line_stream >> srf_name_quoted; // This gets the frame name part
            current_frame.name = unquoteString(srf_name_quoted);
            current_frame.ak.name = current_frame.name;
        } else if (keyword == "FIL") {
            std::string fil_mesh_name_ext;
            line_stream >> fil_mesh_name_ext;
            current_frame.mhId = getFileBaseName(fil_mesh_name_ext);
        } else if (keyword == "CLA") {
            line_stream >> current_frame.ak.cla;
        } else if (keyword == "STA") {
            af3 temp_pos;
            ai3 temp_tpb;
            bool temp_disp = false;
            line_stream >> temp_pos[0] >> temp_pos[1] >> temp_pos[2]
                        >> temp_tpb[0] >> temp_tpb[1] >> temp_tpb[2]
                        >> temp_disp;

            for (int i = 0; i < 3; ++i) {
                if (temp_pos[i] > DNM_THRESHOLD_MAX_POS) temp_pos[i] = DNM_THRESHOLD_MAX_POS;
                else if (temp_pos[i] < -DNM_THRESHOLD_MAX_POS) temp_pos[i] = -DNM_THRESHOLD_MAX_POS;
                temp_pos[i] *= DNM_SCALE_FACTOR;
            }
            temp_tpb[2] *= -1;

            current_frame.ak.poss.push_back(temp_pos);
            current_frame.ak.tpbs.push_back(temp_tpb);
            current_frame.ak.disps.push_back(temp_disp);

        } else if (keyword == "POS") {
            line_stream >> current_frame.pos[0] >> current_frame.pos[1] >> current_frame.pos[2]
                        >> current_frame.tpb[0] >> current_frame.tpb[1] >> current_frame.tpb[2]
                        >> current_frame.disp;
            for (int i = 0; i < 3; ++i) current_frame.pos[i] *= DNM_SCALE_FACTOR;
            current_frame.tpb[2] *= -1;

        } else if (keyword == "CNT") {
            line_stream >> current_frame.cnt.x >> current_frame.cnt.y >> current_frame.cnt.z;
            current_frame.cnt = current_frame.cnt * DNM_SCALE_FACTOR;
        } else if (keyword == "REL") {
            std::string rel_type;
            line_stream >> rel_type;
            if (rel_type != "DEP") {
                std::cout << "line:" << std::right << std::setw(6) << line_idx
                          << " Unexpected REL type '" << rel_type << "' in SRF: " << current_frame.name << std::endl;
            }
        } else if (keyword == "CLD") {
            std::string cld_name_quoted;
            line_stream >> cld_name_quoted;
            std::string child_name = unquoteString(cld_name_quoted);

            if (!frs.frMap.count(child_name)) {
                 SFrame new_child_frame(*this);
                 new_child_frame.name = child_name;
                 frs.frMap[child_name] = new_child_frame;
            }
            frs.frMap[child_name].nested = true;
            current_frame.frIds.push_back(child_name);
        } else if (keyword == "END") {
            if (onSrfSection == E_flib::YES) {
                processAndStoreCurrentFrame(current_frame);
                current_frame.clear();
                onSrfSection = E_flib::NOT;
            }
        }
         if (iss.eof()) { // Check EOF after processing line
            if(onSrfSection == E_flib::YES && !current_frame.name.empty()){
                 processAndStoreCurrentFrame(current_frame); // Process last frame if file ends
            }
            break;
        }
    }
}


/**
 * @brief Processes and stores the currently parsed SFrame object.
 * This includes updating its transform, handling potential merges, and adding to collections.
 * @param frame_to_process The SFrame object to process and store.
 */
void CDnmConvX::processAndStoreCurrentFrame(SFrame& frame_to_process) {
    if (frame_to_process.name.empty()) return;

    frame_to_process.update_transform_and_animation_center();

    bool merged_away = false;
    if (mnm && frame_to_process.frIds.size() == 1 && (frame_to_process.mhId == "null" || frame_to_process.mhId.empty())) {
        const std::string& child_frame_name = frame_to_process.frIds[0];
        if (frs.frMap.count(child_frame_name)) {
            SFrame& child_frame = frs.frMap[child_frame_name];

            SVertex current_ftm_c_vtx = frame_to_process.ftm.c;
            SVertex child_ftm_c_vtx = child_frame.ftm.c;

            bool no_static_transform_change = !current_ftm_c_vtx.anyChange() && !child_ftm_c_vtx.anyChange();

            bool no_anim_pos_change = true;
            for (const auto& anim_pos_af3 : frame_to_process.ak.poss) {
                SVertex anim_vtx = anim_pos_af3;
                if (anim_vtx.anyChange()) {
                    no_anim_pos_change = false;
                    break;
                }
            }

            if (no_static_transform_change && no_anim_pos_change) {
                // std::cout << "Info: Frame '" << frame_to_process.name << "' will be merged into child '" << child_frame_name
                //           << "' due to mnm=true and null mesh." << std::endl;
                child_frame.ak.parents.insert(child_frame.ak.parents.end(),
                                              frame_to_process.ak.parents.begin(), frame_to_process.ak.parents.end());
                child_frame.ak.parents.push_back(frame_to_process.name);
                merged_away = true;
            }
        }
    }

    if (!merged_away) {
        frs << frame_to_process;

        SFrame& stored_frame = frs.frMap[frame_to_process.name];

        aks << stored_frame.ak;

        if (!stored_frame.mhId.empty() && stored_frame.mhId != "null") {
            if (mhs.mhMap.count(stored_frame.mhId)) {
                SMesh& original_mesh = mhs.mhMap[stored_frame.mhId];

                if (original_mesh.pcnt && (*original_mesh.pcnt != stored_frame.cnt)) {
                    SMesh cloned_mesh_instance(original_mesh);

                    std::string clone_name_prefix = stored_frame.name + "_";
                    cloned_mesh_instance.name = clone_name_prefix + original_mesh.name;
                    int instance_count = 0;
                    std::string final_cloned_name = cloned_mesh_instance.name;
                    while(mhs.mhMap.count(final_cloned_name)){ // Ensure unique name
                        final_cloned_name = cloned_mesh_instance.name + "_" + std::to_string(++instance_count);
                    }
                    cloned_mesh_instance.name = final_cloned_name;

                    cloned_mesh_instance.pcnt = &stored_frame.cnt;
                    mhs << cloned_mesh_instance;
                    stored_frame.mhId = cloned_mesh_instance.name;
                } else if (!original_mesh.pcnt) {
                    original_mesh.pcnt = &stored_frame.cnt;
                }


                if (!nstmh) {
                    if (mhs.mhMap.count(stored_frame.mhId)) {
                         omhs << mhs.mhMap[stored_frame.mhId];
                    }
                }
            } else {
                std::cerr << "Error: Mesh ID '" << stored_frame.mhId << "' referenced by frame '"
                          << stored_frame.name << "' not found in parsed meshes." << std::endl;
            }
        }
    }
}


/**
 * @brief Finalizes data after all input files (DNM, INI) have been parsed.
 * This involves processing normals, applying blacklists, face inversions,
 * and other configurations specified in INI files.
 */
void CDnmConvX::finalizeData() {
    // --- Process Normals for All Loaded Meshes ---
    for (auto& [mesh_name, mesh_obj] : mhs.mhMap) {
        if (!mesh_obj.name.empty() && !mesh_obj.vts.empty()) {
            mesh_obj.checkNormal(mesh_obj.name);
        }
    }

    // --- Apply Face Inversions by Index (from INI) ---
    for (const auto& inv_idx_config_str : invfidx) {
        std::istringstream config_stream(inv_idx_config_str);
        std::string mesh_name_to_invert;
        std::uint16_t face_idx_to_invert;
        if (!(config_stream >> mesh_name_to_invert >> face_idx_to_invert)) continue;

        auto mesh_iter = mhs.mhMap.find(mesh_name_to_invert);
        if (mesh_iter != mhs.mhMap.end()) {
            if (face_idx_to_invert < mesh_iter->second.fcs.size()) {
                mesh_iter->second.invertFace(face_idx_to_invert);
            } else {
                std::cerr << "Warning: Face index " << face_idx_to_invert << " out of bounds for mesh '"
                          << mesh_name_to_invert << "' during INI face inversion by index." << std::endl;
            }
        } else {
            // std::cerr << "Warning: Mesh not found for face inversion by index: " << mesh_name_to_invert << std::endl;
        }
    }

    // --- Apply Face Inversions by Material (from INI) ---
    for (const auto& inv_mat_config_str : invfmt) {
        std::istringstream config_stream(inv_mat_config_str);
        std::string mesh_name_to_invert;
        std::string material_name_to_invert;
        if (!(config_stream >> mesh_name_to_invert >> material_name_to_invert)) continue;

        auto mesh_iter = mhs.mhMap.find(mesh_name_to_invert);
        if (mesh_iter != mhs.mhMap.end()) {
            mesh_iter->second.invertFace(material_name_to_invert);
        } else {
            // std::cerr << "Warning: Mesh not found for face inversion by material: " << mesh_name_to_invert << std::endl;
        }
    }

    // --- Apply Mesh Blacklist (from INI) ---
    for (const auto& mesh_name_to_blacklist : mhbl) {
        if (mhs.mhMap.count(mesh_name_to_blacklist)) {
             mhs.mhMap.erase(mesh_name_to_blacklist);
        }
        if (omhs.mhMap.count(mesh_name_to_blacklist)) {
            omhs.mhMap.erase(mesh_name_to_blacklist);
            // std::cout << "Info: Mesh '" << mesh_name_to_blacklist << "' blacklisted from output via INI." << std::endl;
        }
    }

    // --- Apply Frame Blacklist (from INI) ---
    for (const auto& frame_name_to_blacklist : frbl) {
        if (frs.frMap.count(frame_name_to_blacklist)) {
            frs.frMap.erase(frame_name_to_blacklist);
        }
        if (aks.akMap.count(frame_name_to_blacklist)) {
            aks.akMap.erase(frame_name_to_blacklist);
            // std::cout << "Info: Frame '" << frame_name_to_blacklist << "' and its animations blacklisted via INI." << std::endl;
        }
    }

    // --- Process Double-Sided Mesh Materials (from INI) ---
    if (!dsmh.empty()) {
        // std::cout << "// FinalizeData: Processing " << dsmh.size() << " DoubleSideMesh entries." << std::endl;
        for (const auto& material_name_ds : dsmh) {
             auto mat_it = omts.mtMap.find(material_name_ds);
             if (mat_it != omts.mtMap.end()) {
                 // std::cout << "// Material '" << material_name_ds << "' marked for double-sided (if supported by .X material block)." << std::endl;
             }
        }
    }

    // --- Finalize Animation Key Calculations (Post-INI) ---
    for (auto& [frame_name, frame_obj] : frs.frMap) {
        if (frame_obj.ak.name == frame_name) {
            frame_obj.ak.calcSelf();
        }
    }

    if (!nstmh) {
        omhs.mhMap.clear();
        for (const auto& pair_ : mhs.mhMap) {
            bool is_blacklisted = false;
            for(const auto& bl_name : mhbl) if(pair_.first == bl_name) is_blacklisted = true;
            if(is_blacklisted) continue;

            omhs.mhMap.insert(pair_);
        }
    }
     if (!nstmt) {
        omts.mtMap.clear();
        std::map<std::string, SMaterial> materials_in_use;

        auto collect_materials_from_mesh = [&](const SMesh& mesh_to_scan) {
            for (const auto& mat_idx_name : mesh_to_scan.mlist.mtIdx) {
                 if (mts.mtMap.count(mat_idx_name)) {
                     materials_in_use[mat_idx_name] = mts.mtMap.at(mat_idx_name);
                 } else if (mesh_to_scan.mlist.mtMap.count(mat_idx_name)) {
                     materials_in_use[mat_idx_name] = mesh_to_scan.mlist.mtMap.at(mat_idx_name);
                 }
            }
        };

        if (nstmh) {
            for (const auto& frame_pair : frs.frMap) {
                 bool is_frame_blacklisted = false;
                 for(const auto& bl_name : frbl) if(frame_pair.first == bl_name) is_frame_blacklisted = true;
                 if(is_frame_blacklisted) continue;

                if (!frame_pair.second.mhId.empty() && frame_pair.second.mhId != "null") {
                    if (mhs.mhMap.count(frame_pair.second.mhId)) {
                        collect_materials_from_mesh(mhs.mhMap.at(frame_pair.second.mhId));
                    }
                }
            }
        } else {
            for (const auto& mesh_pair : omhs.mhMap) {
                 collect_materials_from_mesh(mesh_pair.second);
            }
        }
        omts.mtMap = materials_in_use;
    }
}

/**
 * @brief Converts the loaded and processed model data into a string formatted for a DirectX .x file.
 * This operator is typically called when preparing the data for outputToXFile.
 * @return A string containing the complete .x file content.
 */
std::string CDnmConvX::operator std::string() const {
    std::stringstream x_file_stream;

    // --- X File Header ---
    x_file_stream << "xof 0302txt 0032\n"
                  << "Header{1;0;1;}\n"
                  << "AnimTicksPerSecond{1;}\n\n";

    x_file_stream << omts;
    x_file_stream << omhs;
    x_file_stream << frs;
    x_file_stream << aks;

    return x_file_stream.str();
}

/**
 * @brief Outputs the converted model to a DirectX .x file.
 * @param outPath The file system path where the .x file will be written.
 *                If empty, a default name is generated based on the input DNM file name.
 * @return An E_ERROR code indicating success (E_NotError) or failure (e.g., E_XWrite).
 */
std::uint16_t CDnmConvX::outputToXFile(const char* outPath) {
    std::string output_file_path_str;
    if (outPath == nullptr || std::string(outPath).empty()) {
        if (inFilePath.empty()) {
            std::cerr << "Error: No input DNM file path set, cannot determine default output path." << std::endl;
            return static_cast<std::uint16_t>(E_ERROR::E_XWrite);
        }
        std::filesystem::path input_fs_path(inFilePath);
        output_file_path_str = input_fs_path.replace_extension(".x").string();
    } else {
        output_file_path_str = outPath;
    }

    std::ofstream fout(output_file_path_str);
    if (!fout) {
        std::cerr << "Error: Could not open output file for writing: " << output_file_path_str << std::endl;
        return static_cast<std::uint16_t>(E_ERROR::E_XWrite);
    }

    fout << operator std::string();

    if (!fout) {
        std::cerr << "Error: Failed to write all data to output file: " << output_file_path_str << std::endl;
        fout.close();
        return static_cast<std::uint16_t>(E_ERROR::E_XWrite);
    }

    fout.close();
    std::cout << "Successfully converted and wrote to: " << output_file_path_str << std::endl;
    return static_cast<std::uint16_t>(E_ERROR::E_NotError);
}

/**
 * @brief Reads and parses a YSFlight Dynamodel (.dnm) file.
 * The content is read into an internal buffer, then parsed via operator<<.
 * @param path_to_dnm Path to the .dnm file.
 * @return An E_ERROR code indicating success or failure.
 */
std::uint16_t CDnmConvX::inputDnmFile(const char* path_to_dnm) {
    if (path_to_dnm == nullptr || std::string(path_to_dnm).empty()) {
         std::cerr << "Error: No DNM file path provided." << std::endl;
        return static_cast<std::uint16_t>(E_ERROR::E_DnmRead);
    }
    inFilePath = path_to_dnm;

    std::ifstream dnm_file_stream(inFilePath, std::ios_base::binary);
    if (!dnm_file_stream.is_open()) {
        std::cerr << "Error: Could not open DNM file: " << inFilePath << std::endl;
        return static_cast<std::uint16_t>(E_ERROR::E_DnmRead);
    }

    std::stringstream file_content_buffer;
    file_content_buffer << dnm_file_stream.rdbuf();
    dnm_file_stream.close();

    if (file_content_buffer.str().empty()) {
        std::cerr << "Error: DNM file is empty or could not be read: " << inFilePath << std::endl;
        return static_cast<std::uint16_t>(E_ERROR::E_DnmRead);
    }

    std::cout << "DNM file '" << inFilePath << "' read successfully. Size: " << file_content_buffer.str().length() << " bytes." << std::endl;

    std::istringstream dnm_data_stream(file_content_buffer.str());
    *this << dnm_data_stream;

    return static_cast<std::uint16_t>(E_ERROR::E_NotError);
}

/**
 * @brief Reads and parses an INI configuration file.
 * Settings from the INI file customize the DNM to X conversion.
 * @param path_to_ini Path to the .ini file.
 * @return An E_ERROR code indicating success or failure.
 */
std::uint16_t CDnmConvX::inputIniFile(const char* path_to_ini) {
    if (path_to_ini == nullptr || std::string(path_to_ini).empty()) {
        return static_cast<std::uint16_t>(E_ERROR::E_IniPath);
    }

    std::filesystem::path ini_fs_path(path_to_ini);
    if (ini_fs_path.extension() != ".ini" && ini_fs_path.extension() != ".INI") {
        // Allow reading even if extension is not strictly .ini, but could warn.
        // std::cout << "Info: File does not have .ini extension: " << path_to_ini << ". Will attempt to read if it exists." << std::endl;
    }

    std::ifstream ini_file_stream(path_to_ini);
    if (!ini_file_stream) {
        // std::cout << "Info: INI file not found or not readable (this may be optional): " << path_to_ini << std::endl;
        return static_cast<std::uint16_t>(E_ERROR::E_IniRead);
    }

    std::string current_line;
    std::vector<std::string>* current_config_category = nullptr;

    std::map<char, std::vector<std::uint16_t>> pose_keyframes;
    std::map<char, std::vector<std::pair<std::uint16_t, std::uint16_t>>> pose_cla_sta_pairs;
    char current_pose_type = 0;

    while (std::getline(ini_file_stream, current_line)) {
        current_line = trimString(current_line);
        if (current_line.empty() || current_line[0] == '#' || current_line[0] == ';') {
            continue;
        }

        if (current_line[0] == '[') {
            current_config_category = nullptr;
            current_pose_type = 0;
            if (current_line == "[Config]") current_config_category = &configs;
            else if (current_line == "[BlacklistMesh]") current_config_category = &mhbl;
            else if (current_line == "[BlacklistFrame]") current_config_category = &frbl;
            else if (current_line == "[DoubleSideMesh]") current_config_category = &dsmh;
            else if (current_line == "[InvertFaceByIdx]") current_config_category = &invfidx;
            else if (current_line == "[InvertFaceByMaterial]") current_config_category = &invfmt;
            else if (current_line == "[PoseF]") current_pose_type = 'F';
            else if (current_line == "[PoseG]") current_pose_type = 'G';
            else if (current_line == "[PoseB]") current_pose_type = 'B';
        } else {
            if (current_config_category) {
                current_config_category->push_back(current_line);
            } else if (current_pose_type != 0) {
                std::istringstream line_ss(current_line);
                if (current_line.rfind("k ", 0) == 0 || current_line.rfind("k\t", 0) == 0) {
                    char k_char_ignored;
                    line_ss >> k_char_ignored;
                    std::uint16_t keyframe_val;
                    while(line_ss >> keyframe_val) {
                        pose_keyframes[current_pose_type].push_back(keyframe_val);
                    }
                } else {
                    std::uint16_t cla_val, sta_val;
                    if (line_ss >> cla_val >> sta_val) {
                        pose_cla_sta_pairs[current_pose_type].push_back({cla_val, sta_val});
                    }
                }
            }
        }
    }
    ini_file_stream.close();

    for (const auto& config_setting : configs) {
        if (config_setting == "UseNestedMaterial") nstmt = true;
        else if (config_setting == "UseNestedMesh") nstmh = true;
        else if (config_setting == "MergeEmptyFrames") mnm = true;
    }

    for (char pose_char : {'F', 'G', 'B'}) {
        if (pose_keyframes.count(pose_char) && pose_cla_sta_pairs.count(pose_char)) {
            const auto& keyframes_for_pose = pose_keyframes.at(pose_char);
            const auto& cla_sta_list_for_pose = pose_cla_sta_pairs.at(pose_char);

            for (const auto& keyframe_time : keyframes_for_pose) {
                for (const auto& cla_sta_pair : cla_sta_list_for_pose) {
                    otl[keyframe_time][cla_sta_pair.first] = cla_sta_pair.second;
                }
            }
        }
    }
    std::cout << "INI file processed: " << path_to_ini << std::endl;
    return static_cast<std::uint16_t>(E_ERROR::E_NotError);
}
