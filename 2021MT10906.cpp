// ASSIGNMENT 3 PART A

#include <iostream>
#include <unordered_map>
#include <deque>
#include <stack>
#include <algorithm>
#include <iomanip> 
#include <sstream>
#include <string>
#include <unordered_map>
#include <limits>
#include <fstream>

using namespace std;

// Structure created to represent an entry in the Translation Lookaside Buffer (TLB)
struct TLBEntry {
    int page_number;
    int last_used; // when the TLB entry was last accessed (for policies like lru)
};

// Function to compare last used times
bool compare_usage(const TLBEntry &a, const TLBEntry &b) {
    return a.last_used < b.last_used;
}

// The following 'TLB' class contains an array of entries in the translation lookaside buffer
class TLB {
    int size;
    TLBEntry* entries; // Pointer to dynamically allocated array
    int current_count; // To keep track of the number of current entries
public:
    TLB(int s) : size(s), current_count(0) { // Constructor
        entries = new TLBEntry[size]; 
    }

    ~TLB() {
        delete[] entries;
    }

    bool find_entry(int page_number, int current_time) {
        // function to check if the entry being accessed is already present in the TLB 
        for (int i = 0; i < current_count; i++) {
            if (entries[i].page_number == page_number) {
                entries[i].last_used = current_time; // Update last used time
                return true;
            }
        }
        return false;
    }

    void insert_fifo(int page_number, int current_time) {
        // the FIFO (First In First Out) property replaces the entry that was the earliest one to be accessed 
        // note that FIFO doesn't account for the recency of the entries 
        if (current_count >= size) {
            for (int i = 1; i < current_count; i++) {
                entries[i - 1] = entries[i];    // each entry is shifted forward by 1 
            }
            current_count--;
        }
        entries[current_count++] = {page_number, current_time}; // new entry is added 
    }

    void insert_lifo(int page_number, int current_time) {
        // the LIFO (Last In First Out) essentially pops out the entry that came in last
        if (current_count >= size) {
            current_count = current_count-1; // move back by 1, so that the new entry replaces the last entry 
        }
        entries[current_count++] = {page_number, current_time}; // Update new entry
    }

    void insert_optimal(int page_number, int current_time, const unsigned int* accesses, int current_index, int N) {
        // this is the most optimal policy in terms of the number of TLB hits
        // however, it is practically impossible to implement because it requires knowledge of future accesses 
        // it replaces the entry that will be replaced farthest in the future 
        if (current_count >= size) {
            int farthest = current_index; // initialisation
            int replace_index = -1;

            for (int i = 0; i < current_count; i++) {
                auto it = find(accesses+current_index, accesses+N, entries[i].page_number);
                if (it == accesses+N) {
                    replace_index = i;
                    break;
                }
                int dist = it - accesses; // Calculate distance of next access from beginning
                if (dist > farthest) { // Check if this distance is greater than the current farthest distance
                    farthest = dist;
                    replace_index = i; // Update replace index
                    //cout << "entry replaced: " << entries[replace_index].page_number << endl;
                }
            }

            // Shift entries to remove the farthest entry
            for (int i = replace_index; i < current_count - 1; i++) {
                entries[i] = entries[i + 1];
            }
            current_count = current_count-1; // Decrement count
        }
        entries[current_count++] = {page_number, current_time}; // Add new entry
    }
};

class TLB_lru {
    // a separate TLB class has been introduced for LRU to introduce additional modifications 
    int size;
    TLBEntry* entries; // Pointer to dynamically allocated array
    int current_count; // To keep track of the number of current entries
    unordered_map<int, int> page_map; // Hash map to track page numbers and their positions in TLB
public:
    TLB_lru(int s) : size(s), current_count(0) { // Constructor
        entries = new TLBEntry[size]; // Allocate memory for the array
    }

    ~TLB_lru() {
        delete[] entries;
    }

    bool find_entry(int page_number, int current_time) {
        // Check if the page is in the TLB using the map
        if (page_map.find(page_number) != page_map.end()) {
            int index = page_map[page_number]; // Get the index of the entry
            entries[index].last_used = current_time; // Update last used time
            return true;
        }
        return false;
    }

    void insert_lru(int page_number, int current_time) {
        // the LRU (Least Recently Used) policy 
        if (current_count >= size) {
            // Find the least recently used entry
            int lru_index = 0;
            for (int i = 1; i < current_count; i++) {
                if (entries[i].last_used < entries[lru_index].last_used) {
                    lru_index = i; // Update index of least recently used entry
                }
            }
            // Remove the least recently used entry from the map
            page_map.erase(entries[lru_index].page_number);

            // Shift the entries to remove the least recently used entry
            for (int i = lru_index; i < current_count - 1; i++) {
                entries[i] = entries[i + 1];
                // Update the map to reflect the new index
                page_map[entries[i].page_number] = i;
            }
            current_count--; // Decrement count
        }

        // Add the new entry
        entries[current_count] = {page_number, current_time}; // Add new entry
        page_map[page_number] = current_count; // Update the map
        current_count++; // Increment count
    }
};


int main() {
    int T;  // This is the number of test cases
    cin >> T;

    // Process each test case
    for (int j = 0; j < T; j++) {   
        int S, P, K, N; // Address space size, Page Size, TLB Size, # memory accesses
        cin >> S >> P >> K >> N;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        unsigned int* addresses = new unsigned int[N]; // Allocate memory for addresses
        //cout << "N = " << N << endl;
        // Read N addresses
        for (int i = 0; i < N; i++) {
            // Attempt to read the address in hexadecimal format
            if (!(cin >> hex >> addresses[i])) {
                cerr << "Error reading address " << i + 1 << " for testcase " << j + 1 << endl;
                delete[] addresses; // Clean up allocated memory
                return 1; // Exit with error if reading fails
            }
        }
        //cout << "accepted addresses for this testcase" << endl;

        unsigned int* page_nums = new unsigned int[N];
        for (int i = 0; i<N; i++){
            // all the addresses are converted to page numbers
            page_nums[i] = addresses[i] / (P * 1024);
            //cout << page_nums[i] << endl;
        }

        // TLBs are initialized for each policy 
        TLB tlb_fifo(K);
        TLB tlb_lifo(K); 
        TLB_lru tlb_lru(K); 
        TLB tlb_opt(K);

        // Initialize hit counters 
        int fifo_hits = 0;
        int lifo_hits = 0;
        int lru_hits = 0;
        int opt_hits = 0;

        // Iterate entry by entry
        for (int i = 0; i < N; i++) {
            int page_number = addresses[i] / (P * 1024); // Calculate the page number (P is given in kiB, hence 1024)

            // TLB operations
            
            if (tlb_fifo.find_entry(page_number, i)) {
                fifo_hits++;
            } else {
                tlb_fifo.insert_fifo(page_number, i);
            }

            if (tlb_lifo.find_entry(page_number, i)) {
                lifo_hits++;
            } else {
                tlb_lifo.insert_lifo(page_number, i);
            }
            
            if (tlb_lru.find_entry(page_number, i)) {
                lru_hits++;
            } else {
                tlb_lru.insert_lru(page_number, i);
            }

            if (tlb_opt.find_entry(page_number, i)) {
                opt_hits++;
            } else {
                tlb_opt.insert_optimal(page_number, i, page_nums, i, N);
            }
        }
        
        // Output results
        cout << fifo_hits << " " << lifo_hits << " " << lru_hits << " " << opt_hits << endl;
        cin >> dec; // changes input mode back to decimal from hexadecimal

        delete[] addresses; // Clean up allocated memory for addresses
    }

    return 0;
}