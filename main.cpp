#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <climits>
#include <cstdint>

#define MAX_SIZE 256

class BinReader
{
public:
    int k;
    std::ifstream f;
    char x;
    uint32_t i;
    float z;

    BinReader(const char* p) : k(0)
    {
        f.open(p, std::ios::binary);
    }
    // vrne naslednji prebrani bit
    bool readBit()
    {
        if(k == 8)
        {
            readByte();
            k = 0;
        }
        bool bit = (x >> k)&1;
        k++;
        return bit;
    }
    // vrne naslednji prebrani byte
    char readByte()
    {
        f.read((char*)&x, sizeof(char));
        return x;
    }
    // vrne naslednje celo število
    int readInt()
    {
        f.read(reinterpret_cast<char*>(&i), sizeof(int));
        return i;
    }
    // vrne naslednje število z decimalno vejico
    float readFloat()
    {
        f.read(reinterpret_cast<char*>(&z), sizeof(float));
        return z;
    }
};

class BinWriter
{
public:
    int k;
    std::ofstream f;
    char x;
    BinWriter(const char* p) : k(0)
    {
        f.open(p, std::ios::binary);
    }
    ~BinWriter()
    {
        if(k > 0)
        {
            writeByte(x);
        }
        f.close();
    }
    void writeBit(bool b)
    {
        if (k == 8) {
            writeByte(x);
            k = 0;
        }
        x ^= (-b ^ x) & (1 << k);
        k++;
    }
    void writeByte(char x)
    {
        f.write((char*)&x, sizeof(x));
    }
    // za tabelo
    void writeInt(int i)
    {
        f.write((char*)&i, sizeof(int));
    }
    // za kompresijo
    void writeFloat(float z)
   {
        f.write((char*)&z, sizeof(float));
    }

};

struct Symbol
{
    char symbol;
    int frequency;
    std::string code;
};

bool isFreqBigger(Symbol* x,
                  Symbol* y)
{
    return x->frequency > y->frequency;
}

bool isSmaller(int x,
               int y)
{
    return x < y;
}

void shannonFanoRec(int bot,
                    int top,
                    std::vector<Symbol*>& sortedVec)
{
    int leftIndex = bot;
    int rightIndex = top;
    int splitIndex = 0;
    int minDiff = INT_MAX;
    while(leftIndex < rightIndex)
    {
        int leftSum = 0;
        int rightSum = 0;
        int diff = 0;
        // seštevam leve frekvence
        for(int i = bot; i < rightIndex; i++)
        {
            leftSum += sortedVec[i]->frequency;
        }
        // seštevam desne frekvence
        for(int i = rightIndex; i < top; i++)
        {
            rightSum += sortedVec[i]->frequency;
        }
        // absolutna vrednost razlike zato,
        // ker tak tudi če je razlika manjša od 0 (|-2| > 5)
        // je to še vedno najmanjša razlika,
        // ampak vrednost ne sme bit < 0
        diff = abs(leftSum - rightSum);
        if(isSmaller(diff, minDiff))
        {
            minDiff = diff;
            splitIndex = rightIndex;
        }
        rightIndex--;
    }
    for(int i = leftIndex; i < splitIndex; i++)
    {
        sortedVec[i]->code += "0";
    }
    for(int i = splitIndex; i < top; i++)
    {
        sortedVec[i]->code += "1";
    }
    int leftSize = splitIndex - bot;
    int rightSize = top - splitIndex;
    if(leftSize > 1)
    {
        shannonFanoRec(bot, splitIndex, sortedVec);
    }
    if(rightSize > 1)
    {
        shannonFanoRec(splitIndex, top, sortedVec);
    }
}

void initSymbolVec(std::vector<Symbol*>& symbolVec)
{
    symbolVec.clear();
    for(int i = 0; i < MAX_SIZE; i++)
    {
        Symbol* symbol = new Symbol;
        symbol->frequency = 0;
        symbol->symbol = (char)i;
        symbol->code = "";               // morem ugotoviti, če bo kodiran s stringom ali z 8-bitnim številom
        symbolVec.push_back(symbol);
    }
}

void shannonFano(std::vector<unsigned char> charVec,
                 std::vector<Symbol*>& symbolVec,
                 std::vector<Symbol*>& sortedVec)
{
    for(int i = 0; i < charVec.size(); i++)
    {
        symbolVec[(int)charVec[i]]->frequency++;
    }
    // če se simbol v datoteki ne pojavi, ga ne kodiram
    for(auto i: symbolVec)
    {
        if(i->frequency > 0)
        {
            sortedVec.push_back(i);
        }
    }
    // sortiram simbole
    std::sort(sortedVec.begin(), sortedVec.end(), isFreqBigger);
    shannonFanoRec(0, sortedVec.size(), sortedVec);
}

void compressFile(const char* fileName,
                  std::vector<unsigned char> charVec,
                  std::vector<Symbol*> symbolVec,
                  std::vector<Symbol*> sortedVec)
{
    BinReader binReader(fileName);
    std::ifstream myFile(fileName, std::ios::binary);
    if(!myFile.is_open())
    {
        return;
    }
    myFile.seekg(0, std::ios::end);
    unsigned int size = myFile.tellg();
    for(int i = 0; i < size; i++)
    {
        char c = binReader.readByte();
        charVec.push_back(c);
    }
    myFile.close();
    BinWriter binWriter("out.bin");
    // inicializacija novega vektorja
    initSymbolVec(symbolVec);
    // shannon-fano
    shannonFano(charVec, symbolVec, sortedVec);
    // ko je symbolVec napolnjen s frekvencami,
    // potem šele pišem frekvence v datoteko :)
    for(auto& i: symbolVec)
    {
        if(i->frequency > 0)
        {
            binWriter.writeInt(i->frequency);
        }
        else
        {
            binWriter.writeInt(0);
        }
    }
    for(auto& i: charVec)
    {
        for(auto& j: sortedVec)
        {
            if(i == j->symbol)
            {
                std::string str = j->code;
                for(int k = 0; k < str.length(); k++)
                {
                    bool bit;
                    if(str[k] == '0')
                    {
                        bit = false;
                    }
                    if(str[k] == '1')
                    {
                        bit = true;
                    }
                    binWriter.writeBit(bit);
                }
            }
        }
    }
    binWriter.~BinWriter();
}

void decompressFile(const char* fileName,
                    std::vector<unsigned char>& charVec,
                    std::vector<Symbol*>& symbolVec,
                    std::vector<Symbol*>& sortedVec)
{
    BinReader binReader(fileName);
    sortedVec.clear();
    initSymbolVec(symbolVec);
    for(auto& i: symbolVec)
    {
        i->frequency = binReader.readInt();
    }
    binReader.readByte();
    shannonFano(charVec, symbolVec, sortedVec);
    // štejem dolžino binarnega zaporedja kodiranih znakov
    unsigned int len = 0;
    for(auto& i: sortedVec)
    {
        len += i->frequency * i->code.length();
    }
    // DEKODIRANJE
    bool bit[len];
    for(int i = 0; i < len; i++)
    {
        bit[i] = binReader.readBit();
    }
    // najdaljši niz zakodiranega znaka v tabeli
    std::string encodedText = "";
    for(int i = 0; i < len; i++)
    {
        if(bit[i] == 1)
        {
            encodedText += "1";
        }
        else
        {
            encodedText += "0";
        }
    }
    // berem bite
    std::string decodedText = "";
    int i = 0;
    for(i = 0; i < len; i++)
    {
        int n = 0;
        int j = 0;
        while(n < sortedVec.size())
        {
            std::string codeStr = sortedVec[n]->code;
            int m = codeStr.length();
            std::string buffer = encodedText.substr(i, m);
            if(buffer[j] != codeStr[j])
            {
                n++;
                j = 0;
            }
            else
            {
                j++;
                if(j == m)
                {
                    char c = sortedVec[n]->symbol;
                    decodedText += c;
                    j = 0;
                    n = 0;
                    i+=m;
                }
            }
        }
    }
    binReader.~BinReader();
    // length * 8 zato, ker je velikost enega char-a 8 bajtov
    float decodedSize = decodedText.length() * 8;
    // len je velikost kompresirane datoteke
    float compressionRatio = decodedSize / (float)len;
    std::ofstream myFile("out_d.bin", std::ios::binary);
    myFile << decodedText;
    //myFile << "\n";
    //myFile << compressionRatio;
    myFile.close();
}

int main(int argc, char* argv[])
{
    std::vector<unsigned char> charVec;
    std::vector<Symbol*> symbolVec;
    std::vector<Symbol*> sortedVec;
    if(argc > 3)
    {
        return 1;
    }
    if(argv[1][0] == 'c')
    {
        compressFile(argv[2], charVec, symbolVec, sortedVec);
    }
    else if(argv[1][0] == 'd')
    {
        decompressFile(argv[2], charVec, symbolVec, sortedVec);
    }
    return 0;
}