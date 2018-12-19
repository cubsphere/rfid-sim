#include <getopt.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <functional>
#include <fstream>

#include "eom-lee.hpp"
#include "lwr-bound.hpp"
#include "chen.hpp"
#include "chen1.hpp"
#include "chen2.hpp"

using namespace std;

bool help_printed = false;
const char *helpful_string = "rfid-sim [options]\n"
                             "options:\n"
                             "--window NUM or -w NUM: sets initial window size to NUM (default 64)\n"
                             "--tags NUM or -t NUM: sets amount of tags present in the first step to NUM (default 100)\n"
                             "--step NUM or -s NUM: sets amount of tags added per step to NUM (default 100)\n"
                             "--repeat NUM or -r NUM: sets amount of time to simulate all estimators for each step to NUM (default 100)\n"
                             "--maximum NUM or -m NUM: sets maximum amount of tags to NUM (default 1000)\n"
                             "--estimator EST or -e EST: activates simulation of estimator EST (default none)\n"
                             "available estimators are:\n"
                             "eom-lee or el\n"
                             "lower-bound or lb\n"
                             "chen or ch\n"
                             "chen-epsilon-2 or ch2\n"
                             "chen-epsilon-5 or ch5\n"
                             "--all or -a: use all available estimators\n"
                             "--help or -h: display this message\n";

void print_help()
{
    if (!help_printed)
    {
        cout << helpful_string;
        help_printed = true;
    }
}

int initial_window = 64,
    initial_tags = 100,
    repeat = 20,
    step = 100,
    maximum = 1000;

bool use_lb = false;
bool use_el = false;

struct func_with_name
{
    function<int(int, int, int)> func;
    string name;
};

const static func_with_name eom_lee_fwn = {.func = eom_lee, .name = "eom-lee"};
const static func_with_name lwr_bound_fwn = {.func = lwr_bound, .name = "lower-bound"};
const static func_with_name chen_fwn = {.func = chen, .name = "chen"};
const static func_with_name chen_fwn1 = {.func = chen1, .name = "chen-eps-5"};
const static func_with_name chen_fwn2 = {.func = chen2, .name = "chen-eps-2"};

const static int EMPTY = 0;
const static int SUCCESS = 1;

void simulate(minstd_rand gen,
              function<int(int, int, int)> estimator,
              vector<long> &vec_tags,
              vector<long> &vec_slots,
              vector<long> &vec_empties,
              vector<long> &vec_collisions,
              vector<long double> &vec_efficiency,
              vector<long double> &vec_runtime)
{
    int iteration = 0;
    for (int tags = initial_tags; tags <= maximum; tags += step)
    {
        vec_tags[iteration] = tags;
        vec_slots[iteration] = 0;
        vec_empties[iteration] = 0;
        vec_runtime[iteration] = 0;
        vec_collisions[iteration] = 0;

        for (int r = 0; r < repeat; ++r)
        {
            vector<int> window(initial_window, 0);
            int remaining_tags = tags;
            long double avg = 0;
            int runs = 0;
            while (1)
            {
                uniform_int_distribution<> dis(0, window.size() - 1);
                for (int i = 0; i < remaining_tags; ++i)
                {
                    window[dis(gen)] += 1;
                }

                int empties = 0;
                int successes = 0;
                int collisions = 0;
                for (int i = 0; i < window.size(); ++i)
                {
                    if (window[i] == EMPTY)
                        ++empties;
                    else if (window[i] == SUCCESS)
                        ++successes;
                    else
                        ++collisions;
                }

                vec_slots[iteration] += window.size();
                vec_empties[iteration] += empties;
                vec_collisions[iteration] += collisions;

                remaining_tags -= successes;
                if (0 == remaining_tags)
                    break;

                clock_t tStart = clock();
                int newsize = estimator(empties, successes, collisions);
                avg += ((long double)(clock() - tStart)) / ((long double)CLOCKS_PER_SEC / 1000000000);
                ++runs;

                window.resize(newsize);
                for (int i = 0; i < window.size(); ++i)
                {
                    window[i] = 0;
                }
            }
            vec_runtime[iteration] += avg / runs;
        }
        vec_slots[iteration] /= repeat;
        vec_empties[iteration] /= repeat;
        vec_collisions[iteration] /= repeat;
        vec_runtime[iteration] /= repeat;
        vec_efficiency[iteration] = ((long double)tags) / vec_slots[iteration];
        ++iteration;
    }
}

template <class T>
void plot(ofstream &of,
          string title,
          string y_title,
          vector<func_with_name> &estimators,
          vector<vector<long>> &tags,
          vector<vector<T>> &y_vec)
{
    for (int i = 0; i < estimators.size(); ++i)
    {
        of.open(title + "_" + estimators[i].name + ".dat",
                ios::binary | ios::trunc);

        for (int j = 0; j < tags[i].size(); ++j)
        {
            of << tags[i][j] << ' ' << y_vec[i][j] << '\n';
        }
        of.close();
    }
    string cmd = "gnuplot -e \"set grid;set terminal png size 800,600;set key fixed left top vertical Right noreverse enhanced autotitle box lt black linewidth 1.000 dashtype solid;set style data linespoints;set xlabel 'tags';";
    cmd += "set ylabel '" + y_title + "';set output '" + title + ".png';";
    cmd += "plot ";
    for (auto fwn : estimators)
        cmd += "'" + title + "_" + fwn.name + ".dat' title '" + fwn.name + "',"s;
    cmd += "\"";

    int _ = system(cmd.c_str());
}

int main(int argc, char **argv)
{
    vector<func_with_name> estimators;

    int c;
    int option_index = 0;
    static struct option long_options[] = {
        {"window", required_argument, 0, 'w'},
        {"tags", required_argument, 0, 't'},
        {"repeat", required_argument, 0, 'r'},
        {"step", required_argument, 0, 's'},
        {"maximum", required_argument, 0, 'm'},
        {"estimator", required_argument, 0, 'e'},
        {"all", required_argument, 0, 'a'},
        {"help", no_argument, 0, 'h'}};

    while (1)
    {
        c = getopt_long(argc, argv, "w:t:r:s:m:e:ah", long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
        case 'w':
            initial_window = stoi(optarg);
            break;

        case 't':
            initial_tags = stoi(optarg);
            break;

        case 'r':
            repeat = stoi(optarg);
            break;

        case 's':
            step = stoi(optarg);
            break;

        case 'm':
            maximum = stoi(optarg);
            break;

        case 'e':
            if ((strcmp(optarg, "el") == 0) | (strcmp(optarg, "eom-lee") == 0))
                estimators.push_back(eom_lee_fwn);
            else if ((strcmp(optarg, "lb") == 0) | (strcmp(optarg, "lower-bound") == 0))
                estimators.push_back(lwr_bound_fwn);
            else if ((strcmp(optarg, "ch") == 0) | (strcmp(optarg, "chen") == 0))
                estimators.push_back(chen_fwn);
            else if ((strcmp(optarg, "ch2") == 0) | (strcmp(optarg, "chen-epsilon-2") == 0))
                estimators.push_back(chen_fwn);
            else if ((strcmp(optarg, "ch5") == 0) | (strcmp(optarg, "chen-epsilon-5") == 0))
                estimators.push_back(chen_fwn);
            break;

        case 'a':
            estimators.push_back(eom_lee_fwn);
            estimators.push_back(lwr_bound_fwn);
            estimators.push_back(chen_fwn);
            estimators.push_back(chen_fwn1);
            estimators.push_back(chen_fwn2);
            break;

        case 'h':
            print_help();
            return 2;

        case '?':
        default:
            return 1;
        }
    }

    if (argc == 1)
    {
        print_help();
        return 2;
    }

    if (maximum < initial_tags)
    {
        cout << "initial tags lower than maximum tags";
        return 7;
    }

    random_device rd;
    minstd_rand gen(rd());
    long iterations = 1 + (maximum - initial_tags) / step;

    vector<vector<long>> tags(estimators.size(), vector<long>(iterations));
    vector<vector<long>> slots(estimators.size(), vector<long>(iterations));
    vector<vector<long>> empties(estimators.size(), vector<long>(iterations));
    vector<vector<long>> collisions(estimators.size(), vector<long>(iterations));
    vector<vector<long double>> efficiency(estimators.size(), vector<long double>(iterations));
    vector<vector<long double>> runtime(estimators.size(), vector<long double>(iterations));

    for (int i = 0; i < estimators.size(); ++i)
    {
        simulate(gen, estimators[i].func, tags[i], slots[i], empties[i], collisions[i], efficiency[i], runtime[i]);
    }

    ofstream of;
    plot<long>(of, "total_slots", "total slots", estimators, tags, slots);
    plot<long>(of, "emtpy_slots", "empty slots", estimators, tags, empties);
    plot<long>(of, "collisions", "collisions", estimators, tags, collisions);
    plot<long double>(of, "efficiency", "efficiency", estimators, tags, efficiency);
    plot<long double>(of, "runtime", "runtime (ns)", estimators, tags, runtime);

    return 0;
}