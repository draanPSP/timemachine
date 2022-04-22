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
#include "100_payload.h"
#include "100_tmctrl100.h"
#include "100B_payload.h"
#include "100B_tmctrl100B.h"
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
namespace fs = std::filesystem;

void help(const char* exeName) {
    cout << "Usage: " << exeName << " [OPTION]..." << endl;
    cout << endl;
    cout << "Creates TM firmware directory for the specified version." << endl;
    cout << endl;
    cout << "General options:" << endl;
    cout << "  -h, --help              display this help and exit" << endl;
    cout << "  -v, --verbose           enable verbose mode (mostly for debugging)" << endl;
    cout << "  -t, --tmdir=DIR         path to TM directory" << endl;
    cout << "  -u, --updir=DIR         path to updater pbps" << endl;
    cout << "  -V, --version=VER       TM firmware version. Supported versions are: 1.00, 1.00Bogus, 1.50, 2.00, 2.50, 2.71SE-C" << endl;
    cout << "  -d, --downdaterdir=DIR  path to 1.00 downdater dump. Required for 1.00 and 1.00Bogus firmware installs" << endl;
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

void extract100Ipl(string iplPath, string installPath, bool verbose) {
    
    string logStr;
    string tmpDir = installPath + "/tmp";
    char *buf;

    fs::create_directory(tmpDir); 

    int size = readFile(iplPath, &buf);
    decryptIPL((u8*)buf, size, 100, "psp_ipl.bin", tmpDir, nullptr, 0, verbose, false, logStr);
    std::cout << "Decrypting 1.00 ipl" << logStr << std::endl;  

    fs::copy(tmpDir + "/stage1_psp_ipl.bin", installPath + "/nandipl.bin", fs::copy_options::overwrite_existing);

    fs::remove_all(tmpDir);

    delete[] buf;
}

void extract150Ipl(u8 *dataPsp, u32 size, string installPath, bool verbose) {
    
    string logStr;
    char *outData = new char[0x400000];
    
    // decrypt updater
    int outSize = pspDecryptPRX(dataPsp, (u8 *)outData, size, nullptr, true);
    char *updaterData = new char[outSize];

    // extract ipl from ipl updater module
    cout << "Extracting ipl" << endl;
    outSize = pspDecryptPRX((const u8 *)&outData[0x303100], (u8 *)updaterData, 0x38480, nullptr, true);
    decryptIPL((u8*)&updaterData[0x980], 0x37000, 150, "psp_ipl.bin", installPath + "/PSARDUMPER", nullptr, 0, verbose, false, logStr);
    std::cout << logStr << std::endl;

    fs::copy(installPath + "/PSARDUMPER/stage1_psp_ipl.bin", installPath + "/nandipl.bin", fs::copy_options::overwrite_existing);

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

void extractFirmware(string installPath, string upDir, string version, bool deletePsarDumper, bool verbose) {

    char *buf;
    string installerPath = upDir + "/" + version + ".PBP";

    int size = readFile(installerPath, &buf);
    if (size > 0) {
        cout << "Setting up firmware " << version <<" in " << installPath << endl;
        u32 pspOff = *(u32*)&buf[0x20];
        u32 psarOff = *(u32*)&buf[0x24];

        cout << "Extracting firmware files from update PBP" << endl;
        pspDecryptPSAR((u8*)&buf[psarOff], size - psarOff, installPath, true, nullptr, 0, verbose, false, false);

        if (version.compare("150") == 0) {
            extract150Ipl((u8*)&buf[pspOff], psarOff - pspOff, installPath, verbose);
        }

        if (fs::exists(installPath + "/PSARDUMPER/stage1_psp_ipl.bin"))
            fs::copy(installPath + "/PSARDUMPER/stage1_psp_ipl.bin", installPath + "/nandipl.bin", fs::copy_options::overwrite_existing);

        if (deletePsarDumper)
            fs::remove_all(installPath + "/PSARDUMPER");

        // tidy up
        delete[] buf;
    }
    else
    {
        std::cerr << "Error reading " << installerPath << std::endl;
        exit(1);
    }
        
}

int main(int argc, char *argv[]) {

    static struct option long_options[] = {
        {"help",         no_argument,       0, 'h'},
        {"verbose",      no_argument,       0, 'v'},
        {"tmdir",        required_argument, 0, 't'},
        {"updir",        required_argument, 0, 'u'},
        {"version",      required_argument, 0, 'V'},
        {"downdaterdir", required_argument, 0, 'd'},
        {0,              0,                 0,  0 }
    };
    int long_index;

    string tmDir = "";
    string upDir = ".";
    bool verbose = false;
    string version;
    string downdaterDir;
    int c = 0;
    cout << showbase << internal << setfill('0');
    while ((c = getopt_long(argc, argv, "hvo:t:u:V:d:", long_options, &long_index)) != -1) {
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
            case 'd':
                downdaterDir = string(optarg);
                break;
            default:
                help(argv[0]);
                return 1;
        }
    }

    char* inData;
    char* outData;
    int size;

    if (!fs::exists(tmDir)) {
        std::cerr << "Specified TM directory does not exist!" << std::endl;
        return 1;
    }

    if ((version.compare("1.00Bogus") == 0) || (version.compare("1.00Bogus") == 0))
    {
        if (downdaterDir.empty()) {
            std::cerr << "Downdater required for 1.00 firmware installs" << std::endl;
            return 1;
        }
        if (!fs::exists(downdaterDir)) {
            std::cerr << "Downdater directory not found" << std::endl;
            return 1;
        }
        if (!fs::exists(downdaterDir + "/ipl.bin") || 
            !fs::exists(downdaterDir + "DUMP")) {
            std::cerr << "Downdater directory does not contain expected contents" << std::endl;
            return 1;
        }
    }

    if (version.compare("1.00") == 0) {
        string installDir = tmDir + "/100";
        
        const auto copyOptions = fs::copy_options::overwrite_existing 
                               | fs::copy_options::recursive;

        fs::copy(downdaterDir + "/DUMP/", installDir, copyOptions);
        fs::create_directory(installDir + "registry");

        extract100Ipl(downdaterDir + "/ipl.bin", installDir, verbose);

        WriteFile((installDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
        WriteFile((installDir + "/payload.bin").c_str(), (void*)tm100_payload, sizeof(tm100_payload));
        WriteFile((installDir + "/tmctrl100.prx").c_str(), (void*)tm100_tmctrl100, sizeof(tm100_tmctrl100));
    }
    if (version.compare("1.00Bogus") == 0) {
        string installDir = tmDir + "/100_Bogus";
        extractFirmware(installDir, upDir, "100", true, verbose);

        extract100Ipl(downdaterDir + "/ipl.bin", installDir, verbose);

        WriteFile((installDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
        WriteFile((installDir + "/payload.bin").c_str(), (void*)tm100B_payload, sizeof(tm100B_payload));
        WriteFile((installDir + "/tmctrl100_Bogus.prx").c_str(), (void*)tm100B_tmctrl100B, sizeof(tm100B_tmctrl100B));
    }
    else if (version.compare("1.50") == 0) {
        string installDir = tmDir + "/150";
        extractFirmware(installDir, upDir, "150", true, verbose);

        WriteFile((installDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
        WriteFile((installDir + "/payload.bin").c_str(), (void*)tm150_payload, sizeof(tm150_payload));
        WriteFile((installDir + "/tmctrl150.prx").c_str(), (void*)tm150_tmctrl150, sizeof(tm150_tmctrl150));
    }
    else if (version.compare("2.00") == 0) {
        string installDir = tmDir + "/200";
        extractFirmware(installDir, upDir, "200", true, verbose);
        
        WriteFile((installDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
        WriteFile((installDir + "/payload.bin").c_str(), (void*)tm200_payload, sizeof(tm200_payload));
        WriteFile((installDir + "/tmctrl200.prx").c_str(), (void*)tm200_tmctrl200, sizeof(tm200_tmctrl200));
    }
    else if (version.compare("2.50") == 0) {
        string installDir = tmDir + "/250";
        extractFirmware(installDir, upDir, "250", true, verbose);

        WriteFile((installDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
        WriteFile((installDir + "/payload.bin").c_str(), (void*)tm250_payload, sizeof(tm250_payload));
        WriteFile((installDir + "/tmctrl250.prx").c_str(), (void*)tm250_tmctrl250, sizeof(tm250_tmctrl250));
    }
    else if (version.compare("2.71SE-C") == 0) {
        string installDir = tmDir + "/271SE";
        string tmpDir = installDir + "/tmp";

        extractFirmware(installDir, upDir, "150", false, verbose);

        //Decrypt boot config files and insert systemctrl.prx
        updateBtCfg(installDir + "/kd/pspbtcnf_game.txt", "/kd/impose.prx", "/kd/impose.prx\n/kd/systemctrl.prx");
        updateBtCfg(installDir + "/kd/pspbtcnf_updater.txt", "/kd/vshbridge.prx", "/kd/vshbridge.prx\n/kd/systemctrl.prx");

        //Remove unused 1.50 firmware files
        fs::remove_all(installDir + "/vsh/etc/");
        fs::remove_all(installDir + "/vsh/resource/");
        fs::remove(installDir + "/kd/mebooter_umdvideo.prx");
        fs::remove(installDir + "/kd/reboot.prx");

        const char *plugins[11] = { "auth_plugin", "game_plugin", "impose_plugin", "msgdialog_plugin",
                                    "msvideo_plugin", "music_plugin", "opening_plugin", "photo_plugin",
                                    "sysconf_plugin", "update_plugin", "video_plugin" };

        for (int i = 0; i < 11; i++) {
            fs::remove(installDir + "/vsh/module/" + plugins[i] + ".prx");
        }

        WriteFile((installDir + "/tm_sloader.bin").c_str(), (void*)tm_sloader, sizeof(tm_sloader));
        WriteFile((installDir + "/payload.bin").c_str(), (void*)tm271_payload, sizeof(tm271_payload));
        WriteFile((installDir + "/tmctrl150.prx").c_str(), (void*)tm271_tmctrl150, sizeof(tm271_tmctrl150));
        WriteFile((installDir + "/tmctrl271.prx").c_str(), (void*)tm271_tmctrl271, sizeof(tm271_tmctrl271));
        WriteFile((installDir + "/vsh/module/paf.prx").c_str(), (void*)tm271_paf, sizeof(tm271_paf));
        WriteFile((installDir + "/vsh/module/vshmain.prx").c_str(), (void*)tm271_vshmain, sizeof(tm271_vshmain));

        //Setup 271
        extractFirmware(tmpDir, upDir, "271", false, verbose);

        const auto copyOptions = fs::copy_options::overwrite_existing 
                               | fs::copy_options::recursive;

        fs::copy(tmpDir + "/data/", installDir + "/data/", copyOptions);
        fs::copy(tmpDir + "/font/", installDir + "/font/", copyOptions);
        fs::copy(tmpDir + "/kd/", installDir + "/kn/", copyOptions);
        fs::copy(tmpDir + "/vsh/etc/", installDir + "/vsh/etc/", copyOptions);
        fs::copy(tmpDir + "/vsh/module/", installDir + "/vsh/nodule/", copyOptions);
        fs::copy(tmpDir + "/vsh/resource/", installDir + "/vsh/resource/", copyOptions);

        //Decrypt boot config files and insert systemctrl.prx
        updateBtCfg(installDir + "/kn/pspbtcnf.txt", "/kd/modulemgr.prx", "/kd/modulemgr.prx\n/kd/systemctrl.prx");
        updateBtCfg(installDir + "/kn/pspbtcnf_game.txt", "/kd/modulemgr.prx", "/kd/modulemgr.prx\n/kd/systemctrl.prx");
        updateBtCfg(installDir + "/kn/pspbtcnf_updater.txt", "/kd/modulemgr.prx", "/kd/modulemgr.prx\n/kd/systemctrl.prx");

        createSystemCtrl(installDir + "/kd/systemctrl.prx", tmpDir + "/PSARDUMPER/reboot.bin", 
                            tm271_systemctrl150, sizeof(tm271_systemctrl150), 0x678, false);

        createSystemCtrl(installDir + "/kn/systemctrl.prx", installDir + "/PSARDUMPER/reboot.bin", 
                            tm271_systemctrl, sizeof(tm271_systemctrl), 0xa27c, true);

        fs::remove_all(tmpDir);
        fs::remove_all(installDir + "/PSARDUMPER");        
    }
    else {
        cerr << "Unsupported version " << version << "!" << endl;
        help(argv[0]);
    }

    return 0;
}
