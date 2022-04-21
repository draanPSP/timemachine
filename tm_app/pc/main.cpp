#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <regex>
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
#include "200_payload.h"
#include "200_tmctrl200.h"
#include "250_payload.h"
#include "250_tmctrl250.h"
#include "271_payload.h"
#include "271_tmctrl150.h"
#include "271_tmctrl271.h"
#include "271_systemctrl150.h"
#include "271_systemctrl.h"
#include "271_paf.h"
#include "271_vshmain.h"

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
    cout << "  -V, --version=VER  TM firmware version. Supported versions are: 1.50, 2.00, 2.50, 2.71SE-C" << endl;
}

int readFile(string path, char **inData) {
    ifstream inFile (path, ios::in|ios::binary|ios::ate);
    if (!inFile.is_open()) {
        cerr << "Could not open " << path << "!" << endl;
        return -1;
    }

    streampos size = inFile.tellg();
    *inData = new char[size];
    inFile.seekg(0, ios::beg);
    inFile.read(*inData, size);
    inFile.close();
    if (size < 0x30) {
        cerr << "Input file is too small!" << endl;
        return -1;
    }

    return size;
}

void extract150Ipl(u8 *dataPsp, u32 size, string outDir, bool verbose) {
    
    string logStr;
    char *outData = new char[0x400000];
    
    // decrypt updater
    int outSize = pspDecryptPRX(dataPsp, (u8 *)outData, size, nullptr, true);
    char *updaterData = new char[outSize];

    // extract ipl from ipl updater module
    cout << "Extracting ipl" << endl;
    outSize = pspDecryptPRX((const u8 *)&outData[0x303100], (u8 *)updaterData, 0x38480, nullptr, true);
    decryptIPL((u8*)&updaterData[0x980], 0x37000, 150, "psp_ipl.bin", outDir + "/PSARDUMPER", nullptr, 0, verbose, false, logStr);
    std::cout << logStr << std::endl;

    delete[] outData;
    delete[] updaterData;
}

int decryptModule(string modulePath, char** outData) {

    char *inData;
    *outData = new char[0x400000];

    int size = readFile(modulePath, &inData);
    size = pspDecryptPRX((u8 *)inData, (u8 *)*outData, size, nullptr, true);

    delete[] inData;

    return size;
}

void updateBtCfg(string path, const char* regex, const char *replacement) {
    
    char *cfgBuf;
    int cfgSize = decryptModule(path, &cfgBuf);
    cfgBuf[cfgSize] = 0;
    string btCnf(cfgBuf);
    btCnf = std::regex_replace(btCnf, std::regex(regex), replacement);
    WriteFile((path).c_str(), (void*)btCnf.c_str(), btCnf.length());

    delete[] cfgBuf;
}

void createSystemCtrl(string systemCtrlPath, string rebootBinPath, const unsigned char *systemCtrlBuf, int systemCtrlSize, int rebootBinOffset, bool gzip)
{
    char *rebootBuf;
    char *outBuf = new char[systemCtrlSize];
    int rebootSize = readFile(rebootBinPath, &rebootBuf);
    char *compressedBuf = new char[rebootSize];
    int compressedSize;

    compressedSize = deflateCompress((void*)rebootBuf, rebootSize, (void*)compressedBuf, rebootSize, 7, gzip);

    memcpy(outBuf, systemCtrlBuf, systemCtrlSize);
    memcpy(&outBuf[rebootBinOffset], compressedBuf, compressedSize);

    WriteFile((systemCtrlPath).c_str(), (void*)outBuf, systemCtrlSize);

    delete[] rebootBuf;
    delete[] outBuf;
    delete[] compressedBuf;
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
        size = readFile(upDir + "/150.pbp", &inData);
        if (size > 0) {
            cout << "Setting up firmware 150 in " << outDir << endl;
            u32 pspOff = *(u32*)&inData[0x20];
            u32 psarOff = *(u32*)&inData[0x24];

            cout << "Extracting firmware files from update PBP" << endl;
            pspDecryptPSAR((u8*)&inData[psarOff], size - psarOff, outDir, true, nullptr, 0, verbose, false, false);

            extract150Ipl((u8*)&inData[pspOff], psarOff - pspOff, outDir, verbose);
            std::filesystem::copy(outDir + "/PSARDUMPER/stage1_psp_ipl.bin", outDir + "/nandipl.bin", std::filesystem::copy_options::overwrite_existing);

            WriteFile((outDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
            WriteFile((outDir + "/payload.bin").c_str(), (void*)tm150_payload, sizeof(tm150_payload));
            WriteFile((outDir + "/tmctrl150.prx").c_str(), (void*)tm150_tmctrl150, sizeof(tm150_tmctrl150));

            std::filesystem::remove_all(outDir + "/PSARDUMPER");

            // tidy up
            delete[] inData;
        }
        else
            return 1;
    }
    else if (version.compare("2.00") == 0) {
        std::string logStr;
        string outDir = tmDir + "/200";
        size = readFile(upDir + "/200.pbp", &inData);
        if (size > 0) {
            cout << "Setting up firmware 200 in " << outDir << endl;
            u32 pspOff = *(u32*)&inData[0x20];
            u32 psarOff = *(u32*)&inData[0x24];

            cout << "Extracting firmware files from update PBP" << endl;
            pspDecryptPSAR((u8*)&inData[psarOff], size - psarOff, outDir, true, nullptr, 0, verbose, false, false);

            std::filesystem::copy(outDir + "/PSARDUMPER/stage1_psp_ipl.bin", outDir + "/nandipl.bin", std::filesystem::copy_options::overwrite_existing);

            WriteFile((outDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
            WriteFile((outDir + "/payload.bin").c_str(), (void*)tm200_payload, sizeof(tm200_payload));
            WriteFile((outDir + "/tmctrl200.prx").c_str(), (void*)tm200_tmctrl200, sizeof(tm200_tmctrl200));

            std::filesystem::remove_all(outDir + "/PSARDUMPER");

            // tidy up
            delete[] inData;
        }
        else
            return 1;
    }
    else if (version.compare("2.50") == 0) {
        std::string logStr;
        string outDir = tmDir + "/250";
        size = readFile(upDir + "/250.pbp", &inData);
        if (size > 0) {
            cout << "Setting up firmware 250 in " << outDir << endl;
            u32 pspOff = *(u32*)&inData[0x20];
            u32 psarOff = *(u32*)&inData[0x24];

            cout << "Extracting firmware files from update PBP" << endl;
            pspDecryptPSAR((u8*)&inData[psarOff], size - psarOff, outDir, true, nullptr, 0, verbose, false, false);

            std::filesystem::copy(outDir + "/PSARDUMPER/stage1_psp_ipl.bin", outDir + "/nandipl.bin", std::filesystem::copy_options::overwrite_existing);

            WriteFile((outDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
            WriteFile((outDir + "/payload.bin").c_str(), (void*)tm250_payload, sizeof(tm250_payload));
            WriteFile((outDir + "/tmctrl250.prx").c_str(), (void*)tm250_tmctrl250, sizeof(tm250_tmctrl250));

            std::filesystem::remove_all(outDir + "/PSARDUMPER");

            // tidy up
            delete[] inData;
        }
        else
            return 1;
    }
    else if (version.compare("2.71SE-C") == 0) {
        std::string logStr;
        string outDir = tmDir + "/271SE";
        string tmpDir = outDir + "/tmp";
        size = readFile(upDir + "/150.pbp", &inData);
        if (size > 0) {
            cout << "Setting up firmware 271 SE-C in " << outDir << endl;
            u32 pspOff = *(u32*)&inData[0x20];
            u32 psarOff = *(u32*)&inData[0x24];

            cout << "Extracting firmware files from update PBP" << endl;
            pspDecryptPSAR((u8*)&inData[psarOff], size - psarOff, outDir, true, nullptr, 0, verbose, false, false);

            extract150Ipl((u8*)&inData[pspOff], psarOff - pspOff, outDir, verbose);
            std::filesystem::copy(outDir + "/PSARDUMPER/stage1_psp_ipl.bin", outDir + "/nandipl.bin", std::filesystem::copy_options::overwrite_existing);

            //Decrypt boot config files and insert systemctrl.prx
            updateBtCfg(outDir + "/kd/pspbtcnf_game.txt", "/kd/impose.prx", "/kd/impose.prx\n/kd/systemctrl.prx");
            updateBtCfg(outDir + "/kd/pspbtcnf_updater.txt", "/kd/vshbridge.prx", "/kd/vshbridge.prx\n/kd/systemctrl.prx");

            //Remove unused 1.50 firmware files
            std::filesystem::remove_all(outDir + "/vsh/etc/");
            std::filesystem::remove_all(outDir + "/vsh/resource/");
            std::filesystem::remove(outDir + "/kd/mebooter_umdvideo.prx");
            std::filesystem::remove(outDir + "/kd/reboot.prx");

            const char *plugins[11] = { "auth_plugin", "game_plugin", "impose_plugin", "msgdialog_plugin",
                                        "msvideo_plugin", "music_plugin", "opening_plugin", "photo_plugin",
                                        "sysconf_plugin", "update_plugin", "video_plugin" };

            for (int i = 0; i < 11; i++) {
                std::filesystem::remove(outDir + "/vsh/module/" + plugins[i] + ".prx");
            }

            WriteFile((outDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
            WriteFile((outDir + "/payload.bin").c_str(), (void*)tm271_payload, sizeof(tm271_payload));
            WriteFile((outDir + "/tmctrl150.prx").c_str(), (void*)tm271_tmctrl150, sizeof(tm271_tmctrl150));
            WriteFile((outDir + "/tmctrl271.prx").c_str(), (void*)tm271_tmctrl271, sizeof(tm271_tmctrl271));
            WriteFile((outDir + "/vsh/module/paf.prx").c_str(), (void*)tm271_paf, sizeof(tm271_paf));
            WriteFile((outDir + "/vsh/module/vshmain.prx").c_str(), (void*)tm271_vshmain, sizeof(tm271_vshmain));

            // tidy up
            delete[] inData;

            size = readFile(upDir + "/271.pbp", &inData);
            if (size > 0) {
                cout << "Setting up firmware 271 SE-C in " << outDir << endl;
                pspOff = *(u32*)&inData[0x20];
                psarOff = *(u32*)&inData[0x24];

                cout << "Extracting firmware files from update PBP" << endl;
                pspDecryptPSAR((u8*)&inData[psarOff], size - psarOff, tmpDir, true, nullptr, 0, verbose, false, false);

                const auto copyOptions = std::filesystem::copy_options::overwrite_existing 
                                       | std::filesystem::copy_options::recursive;

                std::filesystem::copy(tmpDir + "/data/", outDir + "/data/", copyOptions);
                std::filesystem::copy(tmpDir + "/font/", outDir + "/font/", copyOptions);
                std::filesystem::copy(tmpDir + "/kd/", outDir + "/kn/", copyOptions);
                std::filesystem::copy(tmpDir + "/vsh/etc/", outDir + "/vsh/etc/", copyOptions);
                std::filesystem::copy(tmpDir + "/vsh/module/", outDir + "/vsh/nodule/", copyOptions);
                std::filesystem::copy(tmpDir + "/vsh/resource/", outDir + "/vsh/resource/", copyOptions);

                //Decrypt boot config files and insert systemctrl.prx
                updateBtCfg(outDir + "/kn/pspbtcnf.txt", "/kd/modulemgr.prx", "/kd/modulemgr.prx\n/kd/systemctrl.prx");
                updateBtCfg(outDir + "/kn/pspbtcnf_game.txt", "/kd/modulemgr.prx", "/kd/modulemgr.prx\n/kd/systemctrl.prx");
                updateBtCfg(outDir + "/kn/pspbtcnf_updater.txt", "/kd/modulemgr.prx", "/kd/modulemgr.prx\n/kd/systemctrl.prx");

                createSystemCtrl(outDir + "/kd/systemctrl.prx", tmpDir + "/PSARDUMPER/reboot.bin", 
                                 tm271_systemctrl150, sizeof(tm271_systemctrl150), 0x678, false);

                createSystemCtrl(outDir + "/kn/systemctrl.prx", outDir + "/PSARDUMPER/reboot.bin", 
                                 tm271_systemctrl, sizeof(tm271_systemctrl), 0xa27c, true);

                std::filesystem::remove_all(tmpDir);
                std::filesystem::remove_all(outDir + "/PSARDUMPER");

                // tidy up
                delete[] inData;
            }
            else
                return 1;
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
