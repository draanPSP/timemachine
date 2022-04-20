#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <string.h>
#include <getopt.h>

#include "pspdecrypt_lib.h"
#include "PsarDecrypter.h"
#include "PrxDecrypter.h"
#include <common.h>

#include "tm_sloader.h"
#include "150_payload.h"
#include "150_tmctrl150.h"

using namespace std;

void help(const char* exeName) {
    cout << "Usage: " << exeName << " [OPTION]..." << endl;
    cout << endl;
    cout << "Creates TM firmware directory for the specified version." << endl;
    cout << endl;
    cout << "General options:" << endl;
    cout << "  -h, --help         display this help and exit" << endl;
    cout << "  -v, --verbose      enable verbose mode (mostly for debugging)" << endl;
    cout << "  -t, --tmdir=DIR    path to TM directory" << endl;
    cout << "  -u, --updir=DIR    path to updater pbps" << endl;
    cout << "  -V, --version=VER  TM firmware version. Supported versions are: 1.50" << endl;
}

int readUpdatePbp(string path, char **inData, char **outData) {
    ifstream inFile (path, ios::in|ios::binary|ios::ate);
    if (!inFile.is_open()) {
        cerr << "Could not open " << path << "!" << endl;
        return -1;
    }

    streampos size = inFile.tellg();
    *inData = new char[size];
    *outData = new char[size];
    inFile.seekg(0, ios::beg);
    inFile.read(*inData, size);
    inFile.close();
    if (size < 0x30) {
        cerr << "Input file is too small!" << endl;
        return -1;
    }

    return size;
}

int main(int argc, char *argv[]) {

    static struct option long_options[] = {
        {"help",         no_argument,       0, 'h'},
        {"verbose",      no_argument,       0, 'v'},
        {"tmdir",        required_argument, 0, 't'},
        {"updir",        required_argument, 0, 'u'},
        {"version",      required_argument, 0, 'V'},
        {0,              0,                 0,  0 }
    };
    int long_index;

    string tmDir = "";
    string upDir = ".";
    bool verbose = false;
    string version;
    int c = 0;
    cout << showbase << internal << setfill('0');
    while ((c = getopt_long(argc, argv, "hvo:t:u:V:", long_options, &long_index)) != -1) {
        switch (c) {
            case 'h':
                help(argv[0]);
                return 0;
            case 'v':
                verbose = true;
                break;
            case 't':
                tmDir = string(optarg);
                break;
            case 'u':
                upDir = string(optarg);
                break;
            case 'V':
                version = string(optarg);
                break;
            default:
                help(argv[0]);
                return 1;
        }
    }

    char* inData;
    char* outData;
    int size;

    if (!std::filesystem::exists(tmDir)) {
        std::cerr << "Specified TM directory does not exist!" << std::endl;
        return 1;
    }

    if (version.compare("1.50") == 0) {
        std::string logStr;
        string outDir = tmDir + "/150";
        size = readUpdatePbp(upDir + "/150.pbp", &inData, &outData);
        if (size > 0) {
            cout << "Setting up firmware 150 in " << outDir << endl;
            u32 pspOff = *(u32*)&inData[0x20];
            u32 psarOff = *(u32*)&inData[0x24];

            cout << "Extracting firmware files from update PBP" << endl;
            pspDecryptPSAR((u8*)&inData[psarOff], size - psarOff, outDir, true, nullptr, 0, verbose, false, false);

            // extract updater
            int outSize = pspDecryptPRX((const u8 *)&inData[pspOff], (u8 *)outData, psarOff - pspOff, nullptr, true);
            char *updaterData = new char[outSize];

            // extract ipl from ipl updater module
            cout << "Extracting ipl" << endl;
            outSize = pspDecryptPRX((const u8 *)&outData[0x303100], (u8 *)updaterData, 0x38480, nullptr, true);
            decryptIPL((u8*)&updaterData[0x980], 0x37000, 150, "psp_ipl.bin", outDir + "/PSARDUMPER", nullptr, 0, verbose, false, logStr);
            std::cout << logStr << std::endl;
            std::filesystem::copy(outDir + "/PSARDUMPER/stage1_psp_ipl.bin", outDir + "/nandipl.bin", std::filesystem::copy_options::overwrite_existing);

            WriteFile((outDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
            WriteFile((outDir + "/payload.bin").c_str(), (void*)tm150_payload, sizeof(tm150_payload));
            WriteFile((outDir + "/tmctrl150.prx").c_str(), (void*)tm150_tmctrl150, sizeof(tm150_tmctrl150));

            std::filesystem::remove_all(outDir + "/PSARDUMPER");

            // tidy up
            delete[] inData;
            delete[] outData;
            delete[] updaterData;
        }
        else
            return 1;
    }
    else {
        cerr << "Unsupported version " << version << "!" << endl;
        help(argv[0]);
    }

    return 0;
}
