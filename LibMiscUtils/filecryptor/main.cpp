#include "crypto/Decryptor.h"
#include "crypto/Encryptor.h"
#include "terminal/Common.h"

#include <fstream>
#include <random>

#include <boost/program_options.hpp>

#define CSTR_HELP \
    "help"
#define CSTR_ENC \
    "encrypt"
#define CSTR_DEC \
    "decrypt"
#define CSTR_KEY \
    "key"
#define CSTR_OUT \
    "output"

/**
 * @brief Given an output file stream as the cryption destination, try to open it, if not already openned, with a viable file.
 * @param outf: The file stream object that will hold the output file.
 * @param vm: The program options that holds the user arguments given to this program.
 * @param outputOptionName: The output option name used for this program, e.g. 'output'.
 * @return outf, if the outf is openned already; otherwise, use outf to open the user-specified file indicated by the output option(e.g. --output FileName.txt) and return this stream; otherwise, creates a file and generates a random 10 characters as name, then open and return as file stream.
 */
std::ofstream& openCryptOutputStream(std::ofstream& outf, const boost::program_options::variables_map& vm, const char* outputOptionName = CSTR_OUT)
{
    if (outf.is_open())
        return outf;

    if (outputOptionName && vm.count(outputOptionName)) {
        outf.open(vm[outputOptionName].as<std::string>());
    } else // no output file option specified, create a random named file and write.
    {
        std::string name;
        std::random_device rd; // will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd());

        const size_t sz = 10 + 26; // '0'-'9', 'A'-'Z'
        std::uniform_int_distribution<decltype(sz)> dis(0, sz - 1);

        for (int c = 0; c < 10; c++) {
            auto i = dis(gen);
            name.push_back(i < 10 ? '0' + i : 'A' + (i - 10));
        }

        outf.open(name + ".txt");
    }

    return outf;
}

/**
 * @brief Operates encryption or decryption, depending on the template parameter type, using the given key and name of the input file to be encrypted/decrypted, and then output to a viable file.
 * @param key: The cryptographical key used for the cryption operation.
 * @param inFileName: The name of the input file to be encrypted/decrypted.
 * @param vm: The program options that holds the user arguments given to this program.
 * @param outputOptionName: The output option name used for this program, e.g. 'output'.
 * @return true if the file was successfuly read, encrypted/decrypted and wrote to output file, otherwise false.
 */
template <typename _Cryptor>
bool cryptOut(const std::string& key, const std::string& inFileName, const boost::program_options::variables_map& vm, const char* outputOptionName = CSTR_OUT)
{
    if (key.length() * inFileName.length() == 0)
        return false;

    _Cryptor cr(key);

    std::ifstream inf(inFileName, std::ios_base::in);
    if (!inf.good())
        return false;
    inf >> cr;
    inf.close();

    std::ofstream outf;
    if (!openCryptOutputStream(outf, vm, outputOptionName).good())
        return false;
    outf << cr;
    outf.close();

    return true;
}

/**
 * @brief The test program of LibMiscUtils::crypto. Usage: $./FileCryptor --help
 */
int main(int argc, char* argv[])
{
    namespace po = boost::program_options;

    po::options_description odsc("\nUSAGE: ./FileCryptor [options] <args>\n\n\tThis application encrypts or decrypts a given file and outputs to another file.\n\tOnly one kind of task is allowed in each run.\n\nOPTIONS");
    odsc.add_options() //
        (CSTR_HELP ",h", "Produce this help messages.") //
        (CSTR_ENC ",e", po::value<std::string>(), "Encrypt a file. Mandatory if -d is not specified.") //
        (CSTR_DEC ",d", po::value<std::string>(), "Decrypt a file. Mandatory if -e is not specified.") //
        (CSTR_KEY ",k", po::value<std::string>(), "Key for encryption/decryption. ASCII characters only.") //
        (CSTR_OUT ",o", po::value<std::string>(), "Specify the output file. Otherwise, a output file with a random 10-charactors-name will be created.") //
        ;

    po::variables_map vm;
    try {
        auto&& res = po::parse_command_line(argc, argv, odsc);

        po::store(std::forward<decltype(res)>(res), vm);
        po::notify(vm);
    } catch (const boost::program_options::error& e) {
        cout << endl
             << "Warning: There was at least one invalid option given." << endl;
    }

    if (vm.count(CSTR_HELP)) {
        cout << odsc << endl;
        return 0;
    }

    bool isEnc = vm.count(CSTR_ENC) > 0;
    bool isDec = vm.count(CSTR_DEC) > 0;
    bool bCryptable = isEnc || isDec;

    if (!bCryptable) { // this is not an allowed case anyway, so just print help to notify the user
        cout << odsc << endl;
        return 1;
    }

    std::string key;
    if (vm.count(CSTR_KEY)) {
        key = vm[CSTR_KEY].as<std::string>();
    } else {
        key = "SHIFT123"; // this is the default key value
        cout << "NOTE: Since no -p option was provided, default key value will be used:" << endl
             << '\t' << key << endl
             << "Changing the key is recommended." << endl;
    }

    bool bRes = true;
    // until now our app can only do one kind of cryption at once
    if (isEnc) {
        bRes = cryptOut<shift::crypto::Encryptor>(key, vm[CSTR_ENC].as<std::string>(), vm);
    } else if (isDec) {
        bRes = cryptOut<shift::crypto::Decryptor>(key, vm[CSTR_DEC].as<std::string>(), vm);
    }

    if (!bRes) {
        cout << endl
             << COLOR_WARNING "Warning: "
             << "A file issue occured! Operation was not successful." << NO_COLOR << endl;
        cout << odsc << endl;
    }

    return 0;
}
