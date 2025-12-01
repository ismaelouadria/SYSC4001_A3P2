#include <iostream>
#include <fstream>
#include <string>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

struct SharedInfo {
    int student_id;
    char rubric[5];
    int corrected[5];
    int readyCount;
    int numTA;
};

bool load_rubric(char answers[5]) {
    std::ifstream in("rubric.txt");
    if (!in) return false;
    std::string line;
    int index = 0;
    while (std::getline(in, line) && index < 5) {
        size_t comma = line.find(',');
        if (comma != std::string::npos && comma + 2 < line.size()) {
            answers[index] = line[comma + 2];
            index++;
        }
    }
    return index == 5;
}

int load_student_id(const std::string& path) {
    std::ifstream in(path);
    if (!in) return -1;
    int id;
    in >> id;
    return id;
}

void sleep_random(int min_ms, int max_ms) {
    int range = max_ms - min_ms + 1;
    int delay = min_ms + (std::rand() % range);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
}

void save_rubric(char rubric[5]) {
    std::ofstream out("rubric.txt");
    for (int i = 0; i < 5; i++) {
        out << (i + 1) << ", " << rubric[i] << "\n";
    }
}

int load_next_exam(int current_id) {
    int next_id = current_id + 1;
    std::string filename = "exams/" +
        (next_id < 10   ? "000" + std::to_string(next_id) :
         next_id < 100  ? "00"  + std::to_string(next_id) :
         next_id < 1000 ? "0"   + std::to_string(next_id) :
                          std::to_string(next_id)) + ".txt";

    std::ifstream in(filename);
    if (!in) return 9999;
    int id;
    in >> id;
    return id;
}

void reset_rubric() {
    std::ofstream reset("rubric.txt");
    reset << "1, A\n";
    reset << "2, B\n";
    reset << "3, C\n";
    reset << "4, D\n";
    reset << "5, E\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;
    int numTA = std::atoi(argv[1]);
    if (numTA < 2) numTA = 2;
    reset_rubric();

    int shmid = shmget(IPC_PRIVATE, sizeof(SharedInfo), IPC_CREAT | 0666);
    if (shmid < 0) return 1;

    SharedInfo* shm = (SharedInfo*) shmat(shmid, nullptr, 0);
    if (shm == (void*)-1) return 1;

    char temp[5];
    if (!load_rubric(temp)) return 1;
    for (int i = 0; i < 5; i++) {
        shm->rubric[i] = temp[i];
        shm->corrected[i] = 0;
    }

    shm->student_id = load_student_id("exams/0001.txt");
    shm->readyCount = 0;
    shm->numTA = numTA;

    sem_unlink("/rubric_sem");
    sem_unlink("/mark_sem");
    sem_unlink("/load_sem");
    sem_unlink("/start_sem");

    sem_t* rubric_sem = sem_open("/rubric_sem", O_CREAT, 0666, 1);
    sem_t* mark_sem   = sem_open("/mark_sem",   O_CREAT, 0666, 1);
    sem_t* load_sem   = sem_open("/load_sem",   O_CREAT, 0666, 1);
    sem_t* start_sem  = sem_open("/start_sem",  O_CREAT, 0666, 0);

    if (rubric_sem == SEM_FAILED || mark_sem == SEM_FAILED || load_sem == SEM_FAILED || start_sem == SEM_FAILED) return 1;

    for (int i = 0; i < numTA; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            int TA = i + 1;
            std::srand(getpid());

            std::cout << "TA " << TA << " started correcting exams. Starting with student " << shm->student_id << std::endl;

            sem_wait(load_sem);
            shm->readyCount++;
            bool allReady = (shm->readyCount == shm->numTA);
            sem_post(load_sem);

            if (allReady) {
                for (int r = 0; r < shm->numTA; r++) sem_post(start_sem);
            }

            sem_wait(start_sem);

            int lastStudent = -1;

            while (true) {
                sem_wait(load_sem);
                int current_student = shm->student_id;
                if (current_student == 9999) {
                    sem_post(load_sem);
                    std::cout << "TA " << TA << " detected final exam reached. Exiting." << std::endl;
                    break;
                }
                bool new_exam = (current_student != lastStudent);
                sem_post(load_sem);

                if (new_exam) {
                    sem_wait(rubric_sem);
                    std::cout << "TA " << TA << " is reviewing rubric for student " << current_student << std::endl;
                    for (int j = 0; j < 5; j++) {
                        sleep_random(500, 1000);
                        char oldAnswer = shm->rubric[j];
                        bool shouldCorrect = (std::rand() % 2 == 0);
                        if (shouldCorrect) {
                            shm->rubric[j] = oldAnswer + 1;
                            save_rubric(shm->rubric);
                            std::cout << "TA " << TA << " changed the rubric answer for Q" << (j + 1) << " from " << oldAnswer << " to " << shm->rubric[j] << std::endl;
                        } else {
                            std::cout << "TA " << TA << " kept the answer for Q" << (j + 1) << " as " << oldAnswer << std::endl;
                        }
                    }
                    sem_post(rubric_sem);
                    lastStudent = current_student;
                }

                sem_wait(mark_sem);
                for (int k = 0; k < 5; k++) {
                    if (shm->corrected[k] == 0) {
                        shm->corrected[k] = 1;
                        sleep_random(1000, 2000);
                        std::cout << "TA " << TA << " marked Q" << (k + 1) << " for student " << current_student << std::endl;
                        break;
                    }
                }
                sem_post(mark_sem);

                sem_wait(load_sem);
                current_student = shm->student_id;
                if (current_student == 9999) {
                    sem_post(load_sem);
                    std::cout << "TA " << TA << " detected final exam reached during load check. Exiting." << std::endl;
                    break;
                }

                bool fully_corrected = true;
                for (int l = 0; l < 5; l++) {
                    if (shm->corrected[l] == 0) {
                        fully_corrected = false;
                        break;
                    }
                }

                if (fully_corrected) {
                    int next_id = load_next_exam(shm->student_id);

                    if (next_id == 9999) {
                        shm->student_id = 9999;
                        std::cout << "TA " << TA << " is ending the exam correcting. Final exam reached." << std::endl;
                        sem_post(load_sem);
                        break;
                    } else {
                        shm->student_id = next_id;
                        for (int m = 0; m < 5; m++) shm->corrected[m] = 0;
                        std::cout << "TA " << TA << " loaded the next exam. Correcting the exam for student "
                                  << next_id << std::endl;
                    }
                }

                sem_post(load_sem);
            }

            _exit(0);
        }
    }

    for (int i = 0; i < numTA; i++) wait(nullptr);

    sem_close(rubric_sem);
    sem_close(mark_sem);
    sem_close(load_sem);
    sem_close(start_sem);

    sem_unlink("/rubric_sem");
    sem_unlink("/mark_sem");
    sem_unlink("/load_sem");
    sem_unlink("/start_sem");

    shmdt(shm);
    shmctl(shmid, IPC_RMID, nullptr);

    return 0;
}
