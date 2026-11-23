#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <iomanip>
#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;

struct Task {
    int id;
    std::string label;
    double V;                  // Expected cost of the (possibly composite) task
    double p;                  // Success probability of the (possibly composite) task
    std::vector<int> sequence; // Original task IDs inside this composite task

    double R() const {
        return V / (1.0 - p);  // Merit ratio R(t_i) = V_i / (1 - p_i)
    }
};

class GareySolver {
private:
    int n_original = 0;
    std::map<int, double> orig_V;
    std::map<int, double> orig_p;
    std::vector<std::pair<int, int>> orig_arcs;

    std::map<int, Task> tasks;            // Active nodes in the reduced graph
    std::map<int, std::set<int>> succ;    // succ[u] = immediate successors of u
    std::map<int, std::set<int>> pred;    // pred[u] = immediate predecessors of u

    void addArc(int u, int v) {
        if (u == v) return;
        succ[u].insert(v);
        pred[v].insert(u);
    }

    void removeArc(int u, int v) {
        succ[u].erase(v);
        pred[v].erase(u);
    }

    void removeNode(int u) {
        for (int v : succ[u]) pred[v].erase(u);
        for (int p_node : pred[u]) succ[p_node].erase(u);
        succ.erase(u);
        pred.erase(u);
        tasks.erase(u);
    }

    bool isTerminal(int u) const {
        return succ.at(u).empty();
    }

    bool isInitial(int u) const {
        return pred.at(u).empty();
    }

    // Builds "rostami_kucuk/gr_osX/gr_<filename>.txt" and creates the gr_ folder
    static fs::path makeGrFilePath(const fs::path& input_path) {
        fs::path parent_dir = input_path.parent_path();
        std::string file_name = "gr_" + input_path.filename().string();

        if (!parent_dir.empty()) {
            std::string parent_name = parent_dir.filename().string();
            // Place "gr_os1" inside "rostami_kucuk" alongside "os1"
            fs::path gr_folder = parent_dir.parent_path() / ("gr_" + parent_name);
            fs::create_directories(gr_folder);
            return gr_folder / file_name;
        } else {
            fs::path gr_folder = "gr_output";
            fs::create_directories(gr_folder);
            return gr_folder / file_name;
        }
    }

public:
    bool loadFromFile(const fs::path& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filepath.string() << "\n";
            return false;
        }

        tasks.clear();
        succ.clear();
        pred.clear();
        orig_V.clear();
        orig_p.clear();
        orig_arcs.clear();

        std::string line;
        // Line 1: Number of tasks n
        if (!std::getline(file, line)) return false;
        n_original = std::stoi(line);

        // Line 2: Comma-separated costs V_i
        std::getline(file, line);
        std::stringstream ss_cost(line);
        std::string token;
        std::vector<double> costs;
        while (std::getline(ss_cost, token, ',')) {
            costs.push_back(std::stod(token));
        }

        // Line 3: Comma-separated probabilities p_i
        std::getline(file, line);
        std::stringstream ss_prob(line);
        std::vector<double> probs;
        while (std::getline(ss_prob, token, ',')) {
            probs.push_back(std::stod(token));
        }

        for (int i = 1; i <= n_original; ++i) {
            orig_V[i] = costs[i - 1];
            orig_p[i] = probs[i - 1];
            tasks[i] = Task{i, "t" + std::to_string(i), costs[i - 1], probs[i - 1], {i}};
            succ[i] = {};
            pred[i] = {};
        }

        // Lines 4+: Edges formatted as "u-v"
        while (std::getline(file, line)) {
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            if (line.empty()) continue;

            size_t dash_pos = line.find('-');
            if (dash_pos != std::string::npos) {
                int u = std::stoi(line.substr(0, dash_pos));
                int v = std::stoi(line.substr(dash_pos + 1));
                addArc(u, v);
                orig_arcs.push_back({u, v});
            }
        }
        return true;
    }

    int countArcs() const {
        int count = 0;
        for (const auto& [u, s] : succ) count += s.size();
        return count;
    }

    // Applies Garey (1973) Theorems 3.2, 3.3, 4.1, and 4.2 repeatedly
    void runGareyReductions(bool verbose = true) {
        int step = 1;
        bool changed = true;

        while (changed) {
            changed = false;

            // 1. THEOREM 3.3: Terminal Arc Relaxation / Deletion
            for (const auto& [j, _] : tasks) {
                if (!isTerminal(j)) continue;

                int target_i = -1;
                for (int i : pred[j]) {
                    if (tasks[i].R() < tasks[j].R()) {
                        target_i = i;
                        break;
                    }
                }

                if (target_i != -1) {
                    int i = target_i;
                    if (verbose) {
                        std::cout << "Step " << step++ << " [Thm 3.3 - Relax Terminal Arc]: "
                                  << tasks[i].label << " -> " << tasks[j].label
                                  << " (R=" << tasks[i].R() << " < R=" << tasks[j].R() << ")\n";
                    }
                    removeArc(i, j);
                    for (int p_node : pred[i]) {
                        addArc(p_node, j);
                    }
                    changed = true;
                    break;
                }
            }
            if (changed) continue;

            // 2. THEOREM 4.2 (Dual of 3.3): Initial Arc Relaxation / Deletion
            for (const auto& [i, _] : tasks) {
                if (!isInitial(i)) continue;

                int target_j = -1;
                for (int j : succ[i]) {
                    if (tasks[i].R() < tasks[j].R()) {
                        target_j = j;
                        break;
                    }
                }

                if (target_j != -1) {
                    int j = target_j;
                    if (verbose) {
                        std::cout << "Step " << step++ << " [Thm 4.2 - Relax Initial Arc]: "
                                  << tasks[i].label << " -> " << tasks[j].label
                                  << " (R=" << tasks[i].R() << " < R=" << tasks[j].R() << ")\n";
                    }
                    removeArc(i, j);
                    for (int s_node : succ[j]) {
                        addArc(i, s_node);
                    }
                    changed = true;
                    break;
                }
            }
            if (changed) continue;

            // 3. THEOREM 3.2: Combine Nonterminal t_i with Terminal t_j
            for (const auto& [i, _] : tasks) {
                if (succ[i].empty()) continue;

                bool all_succ_terminal = true;
                int min_j = -1;
                double min_R = 1e18;

                for (int j : succ[i]) {
                    if (!isTerminal(j)) {
                        all_succ_terminal = false;
                        break;
                    }
                    if (tasks[j].R() < min_R) {
                        min_R = tasks[j].R();
                        min_j = j;
                    }
                }

                if (all_succ_terminal && min_j != -1) {
                    int j = min_j;
                    if (tasks[j].R() <= tasks[i].R() && pred[j].size() == 1) {
                        if (verbose) {
                            std::cout << "Step " << step++ << " [Thm 3.2 - Merge Terminal]: "
                                      << tasks[i].label << " + " << tasks[j].label
                                      << " (R_i=" << tasks[i].R() << " >= R_j=" << tasks[j].R() << ")\n";
                        }
                        tasks[i].V = tasks[i].V + tasks[i].p * tasks[j].V;
                        tasks[i].p = tasks[i].p * tasks[j].p;
                        tasks[i].label = "[" + tasks[i].label + "," + tasks[j].label + "]";
                        tasks[i].sequence.insert(tasks[i].sequence.end(),
                                                 tasks[j].sequence.begin(),
                                                 tasks[j].sequence.end());

                        removeNode(j);
                        changed = true;
                        break;
                    }
                }
            }
            if (changed) continue;

            // 4. THEOREM 4.1 (Dual of 3.2): Combine Initial t_i with Noninitial t_j
            for (const auto& [j, _] : tasks) {
                if (pred[j].empty()) continue;

                bool all_pred_initial = true;
                int max_i = -1;
                double max_R = -1.0;

                for (int i : pred[j]) {
                    if (!isInitial(i)) {
                        all_pred_initial = false;
                        break;
                    }
                    if (tasks[i].R() > max_R) {
                        max_R = tasks[i].R();
                        max_i = i;
                    }
                }

                if (all_pred_initial && max_i != -1) {
                    int i = max_i;
                    if (tasks[i].R() >= tasks[j].R() && succ[i].size() == 1) {
                        if (verbose) {
                            std::cout << "Step " << step++ << " [Thm 4.1 - Merge Initial]: "
                                      << tasks[i].label << " + " << tasks[j].label
                                      << " (R_i=" << tasks[i].R() << " >= R_j=" << tasks[j].R() << ")\n";
                        }
                        double combined_V = tasks[i].V + tasks[i].p * tasks[j].V;
                        double combined_p = tasks[i].p * tasks[j].p;
                        std::vector<int> combined_seq = tasks[i].sequence;
                        combined_seq.insert(combined_seq.end(),
                                            tasks[j].sequence.begin(),
                                            tasks[j].sequence.end());

                        tasks[j].V = combined_V;
                        tasks[j].p = combined_p;
                        tasks[j].label = "[" + tasks[i].label + "," + tasks[j].label + "]";
                        tasks[j].sequence = combined_seq;

                        removeNode(i);
                        changed = true;
                        break;
                    }
                }
            }
        }
    }

    std::vector<int> getFinalSequence() const {
        std::map<int, int> indeg;
        std::set<int> unscheduled;
        for (const auto& [id, _] : tasks) {
            indeg[id] = pred.at(id).size();
            unscheduled.insert(id);
        }

        std::vector<int> final_seq;
        while (!unscheduled.empty()) {
            int best_u = -1;
            double best_R = 1e18;

            for (int u : unscheduled) {
                if (indeg[u] == 0 && tasks.at(u).R() < best_R) {
                    best_R = tasks.at(u).R();
                    best_u = u;
                }
            }

            for (int orig_id : tasks.at(best_u).sequence) {
                final_seq.push_back(orig_id);
            }

            unscheduled.erase(best_u);
            for (int v : succ.at(best_u)) {
                indeg[v]--;
            }
        }
        return final_seq;
    }

    double evaluateSequence(const std::vector<int>& seq) const {
        double total_cost = 0.0;
        double cumulative_prob = 1.0;
        for (int id : seq) {
            total_cost += cumulative_prob * orig_V.at(id);
            cumulative_prob *= orig_p.at(id);
        }
        return total_cost;
    }

    void writeOutputs(const fs::path& input_filepath,
                      const fs::path& master_log_filepath,
                      const std::vector<int>& final_seq,
                      double expected_cost) const {
        fs::path gr_filepath = makeGrFilePath(input_filepath);

        // Map remaining node IDs to 1..n_new
        std::map<int, int> old_to_new;
        std::vector<int> new_to_old;
        int idx = 1;
        for (const auto& [old_id, _] : tasks) {
            old_to_new[old_id] = idx++;
            new_to_old.push_back(old_id);
        }
        int n_new = tasks.size();

        // 1. Write the reduced graph file inside rostami_kucuk/gr_osX/gr_<filename>.txt
        std::ofstream gr_out(gr_filepath);
        if (!gr_out.is_open()) {
            std::cerr << "Error: Could not write reduced instance to " << gr_filepath.string() << "\n";
        } else {
            gr_out << n_new << "\n";

            for (int i = 0; i < n_new; ++i) {
                double v = tasks.at(new_to_old[i]).V;
                if (std::abs(v - std::round(v)) < 1e-9) {
                    gr_out << static_cast<long long>(std::round(v));
                } else {
                    gr_out << std::fixed << std::setprecision(6) << v;
                }
                if (i + 1 < n_new) gr_out << ", ";
            }
            gr_out << "\n";

            gr_out << std::fixed << std::setprecision(6);
            for (int i = 0; i < n_new; ++i) {
                gr_out << tasks.at(new_to_old[i]).p;
                if (i + 1 < n_new) gr_out << ", ";
            }
            gr_out << "\n";

            for (int i = 0; i < n_new; ++i) {
                int old_u = new_to_old[i];
                int new_u = i + 1;
                for (int old_v : succ.at(old_u)) {
                    int new_v = old_to_new.at(old_v);
                    gr_out << new_u << "-" << new_v << "\n";
                }
            }
            gr_out.close();
            std::cout << "Created reduced graph file: " << gr_filepath.string() << "\n";
        }

        // 2. Append detailed summary to the shared master log file
        std::ofstream log_out(master_log_filepath, std::ios::app);
        if (!log_out.is_open()) {
            std::cerr << "Error: Could not open master log file " << master_log_filepath.string() << "\n";
            return;
        }

        log_out << "================================================================================\n";
        log_out << "INPUT FILE          : " << input_filepath.string() << "\n";
        log_out << "CREATED OUTPUT FILE : " << gr_filepath.string() << "\n";
        log_out << "--------------------------------------------------------------------------------\n";

        log_out << "[OLD GRAPH INFO] (Nodes: " << n_original << ", Arcs: " << orig_arcs.size() << ")\n";
        log_out << "Old Nodes:\n";
        log_out << std::fixed << std::setprecision(6);
        for (int i = 1; i <= n_original; ++i) {
            double r_val = orig_V.at(i) / (1.0 - orig_p.at(i));
            log_out << "  Node " << std::setw(2) << i
                    << " | V = " << std::setw(10) << orig_V.at(i)
                    << " | p = " << orig_p.at(i)
                    << " | R = " << std::setw(10) << r_val << "\n";
        }

        log_out << "Old Arcs (" << orig_arcs.size() << "): ";
        if (orig_arcs.empty()) {
            log_out << "None\n";
        } else {
            for (size_t i = 0; i < orig_arcs.size(); ++i) {
                log_out << orig_arcs[i].first << "-" << orig_arcs[i].second
                        << (i + 1 < orig_arcs.size() ? ", " : "\n");
            }
        }

        log_out << "--------------------------------------------------------------------------------\n";

        int new_arc_count = countArcs();
        log_out << "[NEW REDUCED GRAPH INFO] (Nodes: " << n_new << ", Arcs: " << new_arc_count << ")\n";
        log_out << "New Nodes (Re-indexed 1 to " << n_new << "):\n";
        for (int i = 0; i < n_new; ++i) {
            int old_id = new_to_old[i];
            int new_id = i + 1;
            const Task& t = tasks.at(old_id);

            std::ostringstream seq_ss;
            seq_ss << "{";
            for (size_t k = 0; k < t.sequence.size(); ++k) {
                seq_ss << t.sequence[k] << (k + 1 < t.sequence.size() ? "," : "");
            }
            seq_ss << "}";

            log_out << "  New Node " << std::setw(2) << new_id
                    << " | V = " << std::setw(10) << t.V
                    << " | p = " << t.p
                    << " | R = " << std::setw(10) << t.R()
                    << " | Label = " << t.label
                    << " | Merged Original Tasks = " << seq_ss.str() << "\n";
        }

        log_out << "New Arcs (" << new_arc_count << "): ";
        if (new_arc_count == 0) {
            log_out << "None\n";
        } else {
            bool first = true;
            for (int i = 0; i < n_new; ++i) {
                int old_u = new_to_old[i];
                int new_u = i + 1;
                for (int old_v : succ.at(old_u)) {
                    int new_v = old_to_new.at(old_v);
                    if (!first) log_out << ", ";
                    log_out << new_u << "-" << new_v
                            << " (" << tasks.at(old_u).label << "->" << tasks.at(old_v).label << ")";
                    first = false;
                }
            }
            log_out << "\n";
        }

        log_out << "--------------------------------------------------------------------------------\n";
        log_out << "Final Sequence      : ";
        for (size_t i = 0; i < final_seq.size(); ++i) {
            log_out << final_seq[i] << (i + 1 < final_seq.size() ? " -> " : "\n");
        }
        log_out << "Expected Cost F(s)  : " << std::fixed << std::setprecision(4) << expected_cost << "\n";
        log_out << "================================================================================\n\n";
        log_out.close();
    }
};

void processSingleFile(const fs::path& filepath, const fs::path& master_log) {
    GareySolver solver;
    if (!solver.loadFromFile(filepath)) return;

    std::cout << "========================================================\n";
    std::cout << "Loaded instance: " << filepath.string() << "\n";
    std::cout << "Initial edges  : " << solver.countArcs() << "\n\n";

    solver.runGareyReductions(true);

    std::cout << "\nRemaining edges after Garey reductions: " << solver.countArcs() << "\n";

    std::vector<int> sequence = solver.getFinalSequence();
    double expected_cost = solver.evaluateSequence(sequence);

    std::cout << "Final Task Sequence: ";
    for (size_t i = 0; i < sequence.size(); ++i) {
        std::cout << sequence[i] << (i + 1 < sequence.size() ? " -> " : "\n");
    }
    std::cout << "Total Expected Cost F(s): " << expected_cost << "\n";

    solver.writeOutputs(filepath, master_log, sequence, expected_cost);
}

int main(int argc, char* argv[]) {
    std::cout << std::fixed << std::setprecision(4);

    // Default target is the entire "rostami_kucuk" directory
    fs::path target_path = (argc > 1) ? fs::path(argv[1]) : fs::path("rostami_kucuk");
    fs::path master_log = (argc > 2) ? fs::path(argv[2]) : fs::path("garey_master_log.txt");

    if (fs::is_directory(target_path)) {
        // Clear old master log before doing a full batch run
        if (fs::exists(master_log)) {
            fs::remove(master_log);
        }

        // Collect all input .txt files inside os1, os2, ..., os6 (skipping any gr_ folders/files)
        std::vector<fs::path> input_files;
        for (const auto& entry : fs::recursive_directory_iterator(target_path)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".txt") continue;

            std::string fname = entry.path().filename().string();
            std::string parent_name = entry.path().parent_path().filename().string();

            // Skip already-created gr_ files or gr_ folders
            if (fname.rfind("gr_", 0) == 0 || parent_name.rfind("gr_", 0) == 0) continue;

            input_files.push_back(entry.path());
        }

        std::sort(input_files.begin(), input_files.end());

        for (const auto& file : input_files) {
            processSingleFile(file, master_log);
        }

        std::cout << "\n========================================================\n";
        std::cout << "Done! Processed " << input_files.size() << " files.\n";
        std::cout << "Master log saved to: " << master_log.string() << "\n";
    } else {
        // Single file mode
        processSingleFile(target_path, master_log);
    }

    return 0;
}