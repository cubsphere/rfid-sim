#include <getopt.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <functional>

#include "eom-lee.hpp"
#include "lwr-bound.hpp"

using namespace std;

bool help_printed = false;
const char *helpful_string = "after reading this helpful string, you learn how to use this program properly.\n";

void print_help()
{
    if (!help_printed)
    {
        cout << helpful_string;
        help_printed = true;
    }
}

int initial_window, initial_tags, repeat, step, maximum;
bool initial_window_defined = false,
     initial_tags_defined = false,
     repeat_defined = false,
     step_defined = false,
     maximum_defined = false;

bool use_lb = false;
bool use_el = false;

struct result
{
    long tags;
    long slots;
    long collisions;
    double efficiency;
    double runtime;
};

const static int EMPTY = 0;
const static int SUCCESS = 1;

void simulate(minstd_rand gen, function<int(int, int, int)> estimator, vector<result> &results)
{
    int iteration = 0;
    for (int tags = initial_tags; tags <= maximum; tags += step)
    {
        int remaining_tags = tags;
        vector<int> window(initial_window, 0);

        results[iteration].tags = tags;
        results[iteration].slots = 0;
        results[iteration].collisions = 0;

        clock_t tStart = clock();
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

            results[iteration].slots += window.size();
            results[iteration].collisions += collisions;

            remaining_tags -= successes;
            if(0 == remaining_tags)
                break;

            int newsize = estimator(empties, successes, collisions);

            window.resize(newsize);
            for (int i = 0; i < window.size(); ++i)
            {
                window[i] = 0;
            }
        }
        results[iteration].runtime = ((double)(clock() - tStart)) / (CLOCKS_PER_SEC / 1000);
        results[iteration].efficiency = ((double)tags) / results[iteration].slots;
        ++iteration;
    }
}

int main(int argc, char **argv)
{
    vector<function<int(int, int, int)>> estimators;

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
            initial_window_defined = true;
            break;

        case 't':
            initial_tags = stoi(optarg);
            initial_tags_defined = true;
            break;

        case 'r':
            repeat = stoi(optarg);
            repeat_defined = true;
            break;

        case 's':
            step = stoi(optarg);
            step_defined = true;
            break;

        case 'm':
            maximum = stoi(optarg);
            maximum_defined = true;
            break;

        case 'e':
            if ((strcmp(optarg, "el") == 0) | (strcmp(optarg, "eom-lee") == 0))
                estimators.push_back(eom_lee);
            else if ((strcmp(optarg, "lb") == 0) | (strcmp(optarg, "lower-bound") == 0))
                estimators.push_back(lwr_bound);
            break;

        case 'a':
            estimators.push_back(eom_lee);
            estimators.push_back(lwr_bound);

        case 'h':
            print_help();
            break;

        case '?':
        default:
            return 1;
        }
    }

    if (argc == 1)
    {
        cout << helpful_string;
        return 2;
    }

    if (!initial_window_defined)
    {
        cout << "initial window size must be defined with '--window window_size' or '-w window_size'\ne.g. '-w 10'";
        return 3;
    }
    else if (!initial_tags_defined)
    {
        cout << "initial tag amount must be defined with '--tags tag_amount' or '-t tag_amount'\ne.g. '-t 10'";
        return 4;
    }
    else if (!repeat_defined)
    {
        cout << "repeated runs for each tag amount must be defined with '--repeat repeats_num' or '-r repeats_num'\ne.g. '-r 1'";
        return 5;
    }
    else if (!step_defined)
    {
        cout << "tag increment amount must be defined with '--step increment' or '-s increment'\ne.g. '-s 5'";
        return 6;
    }
    else if (!maximum_defined)
    {
        cout << "maximum tags undefined - using default value of ((initial_tags) + (tag_increment * 10))\n";
        maximum = initial_tags + step * 10;
    }

    if (maximum < initial_tags)
    {
        cout << "initial tags lower than maximum tags";
        return 7;
    }

    random_device rd;
    minstd_rand gen(rd());
    long iterations = 1 + (maximum - initial_tags) / step;

    for(auto func : estimators)
    {
        vector<result> results(iterations);
        simulate(gen, func, results);
    }

    return 0;
}