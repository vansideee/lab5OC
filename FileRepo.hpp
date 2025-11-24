#pragma once
#include "common.hpp"
#include <fstream>
#include <vector>
#include <iostream>

class FileRepository {
    std::string filename_;
public:
    FileRepository(std::string fn) : filename_(fn) {}

    void seedData(const std::vector<Employee>& emps) {
        std::ofstream file(filename_, std::ios::binary);
        for (const auto& e : emps) {
            file.write((char*)&e, sizeof(e));
        }
    }

    bool get(int id, Employee& out) {
        std::ifstream file(filename_, std::ios::binary);
        Employee e;
        while (file.read((char*)&e, sizeof(e))) {
            if (e.num == id) {
                out = e;
                return true;
            }
        }
        return false;
    }

    void update(const Employee& new_data) {
        std::fstream file(filename_, std::ios::binary | std::ios::in | std::ios::out);
        Employee e;
        while (file.read((char*)&e, sizeof(e))) {
            if (e.num == new_data.num) {
                long pos = -1 * static_cast<long>(sizeof(e));
                file.seekp(pos, std::ios::cur);
                file.write((char*)&new_data, sizeof(new_data));
                return;
            }
        }
    }

    void printAll() {
        std::ifstream file(filename_, std::ios::binary);
        Employee e;
        std::cout << "\n[DATABASE CONTENT]:\n";
        while (file.read((char*)&e, sizeof(e))) {
            std::cout << "  ID:" << e.num << " " << e.name << " " << e.hours << "h\n";
        }
        std::cout << "--------------------\n";
    }
};