// #include "Threads.hpp"
#include <windows.h>

#include "Leaderboard.hpp"
#include "Worker.hpp"


int main() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif

    int num_threads = 8; // Number of logical cores
    Dandelifeon::Archive archive;
    Dandelifeon::Engine engine(100, 60, 50000);
    Dandelifeon::Leaderboard ui(num_threads);

    Dandelifeon::g_thread_mana = std::make_unique<std::atomic<long>[]>(num_threads);
    Dandelifeon::g_thread_blocks = std::make_unique<std::atomic<int>[]>(num_threads);

    for (int i = 0; i < num_threads; i++) {
        Dandelifeon::g_thread_mana[i].store(0);
        Dandelifeon::g_thread_blocks[i].store(0);
    }

    std::vector<std::thread> workers;
    for (int i = 0; i < num_threads; ++i) {
        workers.emplace_back(Dandelifeon::workerTask, i, std::ref(archive), std::cref(engine));
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::vector<long> mana_snap(num_threads);
        std::vector<int> blocks_snap(num_threads);
        for (int i = 0; i < num_threads; i++) {
            mana_snap[i] = Dandelifeon::g_thread_mana[i].load();
            blocks_snap[i] = Dandelifeon::g_thread_blocks[i].load();
        }

        ui.draw(mana_snap, blocks_snap, Dandelifeon::g_total_iters.load());
    }

    return 0;
}