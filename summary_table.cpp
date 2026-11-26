#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <cstdio>

struct Record {
    std::string os_group;
    std::string instance;
    int old_nodes = 0;
    int new_nodes = 0;
    int old_arcs  = 0;
    int new_arcs  = 0;

    double nodeReductionPct() const {
        return (old_nodes > 0) ? (100.0 * (old_nodes - new_nodes) / old_nodes) : 0.0;
    }

    double arcReductionPct() const {
        return (old_arcs > 0) ? (100.0 * (old_arcs - new_arcs) / old_arcs) : 0.0;
    }
};

struct GroupStats {
    int count = 0;
    double sum_old_nodes = 0.0;
    double sum_new_nodes = 0.0;
    double sum_old_arcs  = 0.0;
    double sum_new_arcs  = 0.0;

    void add(const Record& r) {
        count++;
        sum_old_nodes += r.old_nodes;
        sum_new_nodes += r.new_nodes;
        sum_old_arcs  += r.old_arcs;
        sum_new_arcs  += r.new_arcs;
    }

    double avgOldNodes() const { return count ? sum_old_nodes / count : 0.0; }
    double avgNewNodes() const { return count ? sum_new_nodes / count : 0.0; }
    double avgOldArcs()  const { return count ? sum_old_arcs  / count : 0.0; }
    double avgNewArcs()  const { return count ? sum_new_arcs  / count : 0.0; }

    // Aggregate reduction percentage: (Avg Old - Avg New) / Avg Old * 100
    double nodeReductionPct() const {
        return (sum_old_nodes > 0.0) ? (100.0 * (sum_old_nodes - sum_new_nodes) / sum_old_nodes) : 0.0;
    }

    double arcReductionPct() const {
        return (sum_old_arcs > 0.0) ? (100.0 * (sum_old_arcs - sum_new_arcs) / sum_old_arcs) : 0.0;
    }
};

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string extractFilename(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

std::string extractOSGroup(const std::string& path) {
    std::string lower = path;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    for (size_t i = 0; i + 2 < lower.size(); ++i) {
        if (lower[i] == 'o' && lower[i + 1] == 's' && std::isdigit(static_cast<unsigned char>(lower[i + 2]))) {
            size_t j = i + 2;
            while (j < lower.size() && std::isdigit(static_cast<unsigned char>(lower[j]))) {
                j++;
            }
            std::string grp = lower.substr(i, j - i);
            std::transform(grp.begin(), grp.end(), grp.begin(),
                           [](unsigned char c) { return std::toupper(c); });
            return grp;
        }
    }
    return "UNKNOWN";
}

void writeFormattedTable(std::ostream& out,
                         const std::vector<Record>& records,
                         const std::map<std::string, GroupStats>& os_stats,
                         const GroupStats& overall) {
    const std::string sep_heavy(102, '=');
    const std::string sep_light(102, '-');

    out << sep_heavy << "\n";
    out << "GAREY (1973) GRAPH REDUCTION SUMMARY TABLE\n";
    out << sep_heavy << "\n";

    out << std::left  << std::setw(8)  << "OS"
        << std::left  << std::setw(22) << "Instance"
        << std::right << std::setw(11) << "Old Nodes"
        << std::right << std::setw(11) << "New Nodes"
        << std::right << std::setw(13) << "Node Red (%)"
        << std::right << std::setw(11) << "Old Arcs"
        << std::right << std::setw(11) << "New Arcs"
        << std::right << std::setw(13) << "Arc Red (%)"
        << "\n";
    out << sep_light << "\n";

    out << std::fixed << std::setprecision(2);
    for (const auto& r : records) {
        out << std::left  << std::setw(8)  << r.os_group
            << std::left  << std::setw(22) << r.instance
            << std::right << std::setw(11) << r.old_nodes
            << std::right << std::setw(11) << r.new_nodes
            << std::right << std::setw(12) << r.nodeReductionPct() << "%"
            << std::right << std::setw(11) << r.old_arcs
            << std::right << std::setw(11) << r.new_arcs
            << std::right << std::setw(12) << r.arcReductionPct() << "%"
            << "\n";
    }

    out << sep_heavy << "\n";
    out << "SUMMARY BY ORDER STRENGTH (OS1 - OS6)\n";
    out << sep_light << "\n";
    out << std::left  << std::setw(8)  << "Group"
        << std::left  << std::setw(22) << "Instances"
        << std::right << std::setw(11) << "Avg Old N"
        << std::right << std::setw(11) << "Avg New N"
        << std::right << std::setw(13) << "Node Red (%)"
        << std::right << std::setw(11) << "Avg Old A"
        << std::right << std::setw(11) << "Avg New A"
        << std::right << std::setw(13) << "Arc Red (%)"
        << "\n";
    out << sep_light << "\n";

    for (const auto& [os_name, st] : os_stats) {
        out << std::left  << std::setw(8)  << os_name
            << std::left  << std::setw(22) << (std::to_string(st.count) + " graphs")
            << std::right << std::setw(11) << st.avgOldNodes()
            << std::right << std::setw(11) << st.avgNewNodes()
            << std::right << std::setw(12) << st.nodeReductionPct() << "%"
            << std::right << std::setw(11) << st.avgOldArcs()
            << std::right << std::setw(11) << st.avgNewArcs()
            << std::right << std::setw(12) << st.arcReductionPct() << "%"
            << "\n";
    }

    out << sep_light << "\n";
    out << std::left  << std::setw(8)  << "OVERALL"
        << std::left  << std::setw(22) << (std::to_string(overall.count) + " graphs (All OS)")
        << std::right << std::setw(11) << overall.avgOldNodes()
        << std::right << std::setw(11) << overall.avgNewNodes()
        << std::right << std::setw(12) << overall.nodeReductionPct() << "%"
        << std::right << std::setw(11) << overall.avgOldArcs()
        << std::right << std::setw(11) << overall.avgNewArcs()
        << std::right << std::setw(12) << overall.arcReductionPct() << "%"
        << "\n";
    out << sep_heavy << "\n";
}

void writeCSV(const std::string& csv_path,
              const std::vector<Record>& records,
              const std::map<std::string, GroupStats>& os_stats,
              const GroupStats& overall) {
    std::ofstream csv(csv_path);
    if (!csv.is_open()) {
        std::cerr << "Error: Could not write CSV file " << csv_path << "\n";
        return;
    }

    csv << std::fixed << std::setprecision(2);
    csv << "OS,Instance,Old Nodes,New Nodes,Node Reduction (%),Old Arcs,New Arcs,Arc Reduction (%)\n";

    for (const auto& r : records) {
        csv << r.os_group << ","
            << r.instance << ","
            << r.old_nodes << ","
            << r.new_nodes << ","
            << r.nodeReductionPct() << ","
            << r.old_arcs << ","
            << r.new_arcs << ","
            << r.arcReductionPct() << "\n";
    }

    csv << "\nSummary Group,Instance Count,Avg Old Nodes,Avg New Nodes,Node Reduction (%),Avg Old Arcs,Avg New Arcs,Arc Reduction (%)\n";
    for (const auto& [os_name, st] : os_stats) {
        csv << os_name << ","
            << st.count << ","
            << st.avgOldNodes() << ","
            << st.avgNewNodes() << ","
            << st.nodeReductionPct() << ","
            << st.avgOldArcs() << ","
            << st.avgNewArcs() << ","
            << st.arcReductionPct() << "\n";
    }

    csv << "OVERALL (ALL OS),"
        << overall.count << ","
        << overall.avgOldNodes() << ","
        << overall.avgNewNodes() << ","
        << overall.nodeReductionPct() << ","
        << overall.avgOldArcs() << ","
        << overall.avgNewArcs() << ","
        << overall.arcReductionPct() << "\n";

    csv.close();
}

int main(int argc, char* argv[]) {
    std::string log_path = (argc > 1) ? argv[1] : "garey_master_log.txt";
    std::string txt_out_path = "garey_summary_table.txt";
    std::string csv_out_path = "garey_summary_table.csv";

    std::ifstream in(log_path);
    if (!in.is_open()) {
        std::cerr << "Error: Could not open " << log_path << "\n";
        return 1;
    }

    std::vector<Record> records;
    Record current;
    bool has_input = false;
    bool has_old = false;

    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);

        if (line.rfind("INPUT FILE", 0) == 0) {
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                std::string filepath = trim(line.substr(colon + 1));
                current = Record{};
                current.instance = extractFilename(filepath);
                current.os_group = extractOSGroup(filepath);
                has_input = true;
                has_old = false;
            }
        } else if (has_input && line.rfind("[OLD GRAPH INFO]", 0) == 0) {
            if (std::sscanf(line.c_str(), "[OLD GRAPH INFO] (Nodes: %d, Arcs: %d)",
                            &current.old_nodes, &current.old_arcs) == 2) {
                has_old = true;
            }
        } else if (has_input && has_old && line.rfind("[NEW REDUCED GRAPH INFO]", 0) == 0) {
            if (std::sscanf(line.c_str(), "[NEW REDUCED GRAPH INFO] (Nodes: %d, Arcs: %d)",
                            &current.new_nodes, &current.new_arcs) == 2) {
                records.push_back(current);
                has_input = false;
                has_old = false;
            }
        }
    }
    in.close();

    if (records.empty()) {
        std::cerr << "No graph records found in " << log_path << "\n";
        return 1;
    }

    std::map<std::string, GroupStats> os_stats;
    GroupStats overall;
    for (const auto& r : records) {
        os_stats[r.os_group].add(r);
        overall.add(r);
    }

    // Print table to terminal
    writeFormattedTable(std::cout, records, os_stats, overall);

    // Save formatted text table
    std::ofstream txt_out(txt_out_path);
    if (txt_out.is_open()) {
        writeFormattedTable(txt_out, records, os_stats, overall);
        txt_out.close();
        std::cout << "\nSaved text table to: " << txt_out_path << "\n";
    }

    // Save CSV table for Excel
    writeCSV(csv_out_path, records, os_stats, overall);
    std::cout << "Saved CSV table to : " << csv_out_path << "\n";

    return 0;
}