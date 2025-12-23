#include "alo/types.hpp"

#include <chrono>
#include <cstdint>
#include <cerrno>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace {

struct PerftJob {
    int line_no = 0;
    int depth = 0;
    std::string fen;
};

static bool ensure_data_dir() {
#ifdef _WIN32
    // Best-effort; if it already exists, fine.
    return _mkdir("data") == 0 || errno == EEXIST;
#else
    struct stat st;
    if (stat("data", &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return mkdir("data", 0755) == 0;
#endif
}

static std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) start++;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) end--;
    return s.substr(start, end - start);
}

static std::string current_timestamp_local() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

static std::string csv_escape(const std::string& s) {
    bool needs_quotes = false;
    for (char c : s) {
        if (c == '"' || c == ',' || c == '\n' || c == '\r') {
            needs_quotes = true;
            break;
        }
    }
    if (!needs_quotes) return s;
    std::string out;
    out.reserve(s.size() + 2);
    out.push_back('"');
    for (char c : s) {
        if (c == '"') out += "\"\"";
        else out.push_back(c);
    }
    out.push_back('"');
    return out;
}

static bool parse_int_strict(const std::string& s, int* out) {
    if (!out) return false;
    std::string t = trim(s);
    if (t.empty()) return false;
    char* end = nullptr;
    long v = std::strtol(t.c_str(), &end, 10);
    if (!end || *end != '\0') return false;
    if (v < 0 || v > 1000000) return false;
    *out = static_cast<int>(v);
    return true;
}

static bool looks_like_fen(const std::string& s) {
    // Cheap heuristic: piece placement contains '/' and there are spaces separating fields.
    return s.find('/') != std::string::npos && s.find(' ') != std::string::npos;
}

static bool parse_line_to_job(const std::string& raw, int line_no, PerftJob* job_out, std::string* err) {
    if (!job_out) return false;
    std::string line = trim(raw);
    if (line.empty()) return false;
    if (line[0] == '#') return false;

    // Strip trailing comments: everything after '#'
    if (auto hash = line.find('#'); hash != std::string::npos) {
        line = trim(line.substr(0, hash));
        if (line.empty()) return false;
    }

    PerftJob job;
    job.line_no = line_no;

    // Supported formats:
    // 1) "<depth> <fen...>"
    // 2) "<fen...> ; <depth>"
    // 3) "<fen...> <depth>" (depth as last token)
    {
        auto semi = line.find(';');
        if (semi != std::string::npos) {
            std::string left = trim(line.substr(0, semi));
            std::string right = trim(line.substr(semi + 1));
            int depth = 0;
            if (!parse_int_strict(right, &depth)) {
                if (err) *err = "could not parse depth after ';'";
                return false;
            }
            if (!looks_like_fen(left)) {
                if (err) *err = "left side does not look like a FEN";
                return false;
            }
            job.depth = depth;
            job.fen = left;
            *job_out = std::move(job);
            return true;
        }
    }

    // Tokenize whitespace
    std::istringstream iss(line);
    std::vector<std::string> tokens;
    for (std::string t; iss >> t;) tokens.push_back(t);
    if (tokens.size() < 2) {
        if (err) *err = "expected at least depth and FEN";
        return false;
    }

    // Case 1: first token is depth, remainder is fen
    int depth_first = 0;
    if (parse_int_strict(tokens[0], &depth_first)) {
        std::string fen;
        for (size_t i = 1; i < tokens.size(); ++i) {
            if (i > 1) fen.push_back(' ');
            fen += tokens[i];
        }
        fen = trim(fen);
        if (!looks_like_fen(fen)) {
            if (err) *err = "FEN does not look valid after depth";
            return false;
        }
        job.depth = depth_first;
        job.fen = std::move(fen);
        *job_out = std::move(job);
        return true;
    }

    // Case 3: last token is depth, preceding tokens form fen
    int depth_last = 0;
    if (parse_int_strict(tokens.back(), &depth_last)) {
        std::string fen;
        for (size_t i = 0; i + 1 < tokens.size(); ++i) {
            if (i > 0) fen.push_back(' ');
            fen += tokens[i];
        }
        fen = trim(fen);
        if (!looks_like_fen(fen)) {
            if (err) *err = "FEN does not look valid before depth";
            return false;
        }
        job.depth = depth_last;
        job.fen = std::move(fen);
        *job_out = std::move(job);
        return true;
    }

    if (err) *err = "could not find a depth token (supported: depth first, depth last, or '; depth')";
    return false;
}

static inline uint64_t perft_count(Board* pos, int depth) {
    if (depth <= 0) return 1;

    MoveList list[1];
    GenerateAllMoves(pos, list);

    if (depth == 1) {
        uint64_t nodes = 0;
        for (int i = 0; i < list->count; ++i) {
            if (!MakeMove(pos, list->moves[i].move)) continue;
            nodes++;
            TakeMove(pos);
        }
        return nodes;
    }

    uint64_t nodes = 0;
    for (int i = 0; i < list->count; ++i) {
        if (!MakeMove(pos, list->moves[i].move)) continue;
        nodes += perft_count(pos, depth - 1);
        TakeMove(pos);
    }
    return nodes;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input.txt> [output.csv]\n"
                  << "Input formats (one per line, comments with #):\n"
                  << "  <depth> <FEN...>\n"
                  << "  <FEN...> ; <depth>\n"
                  << "  <FEN...> <depth>\n";
        return 2;
    }

    const std::string input_path = argv[1];
    const std::string output_path = (argc >= 3) ? argv[2] : "data/perft_suite.csv";

    std::ifstream in(input_path);
    if (!in) {
        std::cerr << "Failed to open input file: " << input_path << "\n";
        return 2;
    }

    if (!ensure_data_dir()) {
        std::cerr << "Failed to ensure output directory 'data/' exists.\n";
        return 2;
    }

    AllInit();

    bool need_header = true;
    {
        std::ifstream existing(output_path, std::ios::binary);
        if (existing) {
            existing.seekg(0, std::ios::end);
            if (existing.tellg() > 0) need_header = false;
        }
    }

    std::ofstream out(output_path, std::ios::app);
    if (!out) {
        std::cerr << "Failed to open output file for append: " << output_path << "\n";
        return 2;
    }

    if (need_header) {
        out << "timestamp,suite,idx,line,depth,nodes,time_ms,nps,fen\n";
    }

    std::string line;
    int line_no = 0;
    int idx = 0;
    uint64_t total_nodes = 0;
    uint64_t total_time_ms = 0;

    const std::string timestamp = current_timestamp_local();

    while (std::getline(in, line)) {
        line_no++;
        PerftJob job;
        std::string err;
        if (!parse_line_to_job(line, line_no, &job, &err)) {
            continue;
        }

        idx++;
        Board pos[1];
        if (ParseFen(job.fen.c_str(), pos) != 0) {
            out << csv_escape(timestamp) << "," << csv_escape(input_path) << "," << idx << "," << job.line_no << ","
                << job.depth << "," << 0 << "," << 0 << "," << 0 << "," << csv_escape(job.fen) << "\n";
            continue;
        }

        auto t0 = std::chrono::steady_clock::now();
        uint64_t nodes = perft_count(pos, job.depth);
        auto t1 = std::chrono::steady_clock::now();

        uint64_t time_ms = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        double seconds = (double)time_ms / 1000.0;
        double nps = (seconds > 0.0) ? ((double)nodes / seconds) : 0.0;

        total_nodes += nodes;
        total_time_ms += time_ms;

        out << csv_escape(timestamp) << "," << csv_escape(input_path) << "," << idx << "," << job.line_no << ","
            << job.depth << "," << nodes << "," << time_ms << "," << std::fixed << std::setprecision(0) << nps << ","
            << csv_escape(job.fen) << "\n";
    }

    // Append a roll-up row for this suite run.
    double total_seconds = (double)total_time_ms / 1000.0;
    double total_nps = (total_seconds > 0.0) ? ((double)total_nodes / total_seconds) : 0.0;
    out << csv_escape(timestamp) << "," << csv_escape(input_path) << "," << 0 << "," << 0 << "," << -1 << ","
        << total_nodes << "," << total_time_ms << "," << std::fixed << std::setprecision(0) << total_nps << ","
        << csv_escape("__TOTAL__") << "\n";

    return 0;
}
