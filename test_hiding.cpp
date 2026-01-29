#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <time.h>
#include <cmath>
#include <cstdlib>
#include <functional> //vector<reference_wrapper>
#include <queue>
#include <list>
#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <memory> // for unique_ptr

using namespace std;
/*
- 投影點夠扣、不夠扣設為1、Node和DB資料一致完成
- 當一次做a、b時，因為a的Node可能被b修改，因此Node a的資訊未必是最新的(但能保證不超過門檻)
    : 如1,2,4,4在Idx 1、4、6、9，如果只做a這顆子樹，則Idx 9的資訊與最後DB是一致的；
    但做a、b時，1,2,4,4的資訊會因為2,4的修改，而與DB不一致。
    但不影響輸出結果，頂多因為被其他Node修改(如b)，使Node a紀錄的utility比實際上多但不超過門檻
    (*尚未往上層找投影點扣，因此此版Node a的樹有超過門檻的情況發生)
*/

/*
若Pattern有2個Seq:
一個Seq無法再被減(所有Iu皆為1)，仍有應減去而未減去的Utility
一個雖仍有Iu可減，但已減掉該Seq應減去的Utility
此時會導致Hiding failure
(如Pattern 1-2,4,4)

解決:若仍有Utility應減去而未減，再次去其他Seq減?
*/
class SeqData
{
public:
    int sid;
    vector<int> IndexArray;
    vector<int> TidArray;
    vector<int> ItemArray;
    vector<int> IuArray; // new
    vector<double> UtilityArray;
    vector<double> RuArray;

    map<int, vector<int>> ItemIdxTable; //<Item,[Idx set]>
    vector<int> IdxTid;                 // size n代表Tid n、<Tid起始Idx>
};
vector<SeqData> VecDataBase;

class L1_UtilityInfo
{
public:
    vector<int> VecIndex;
    // vector<int> VecTid;
    vector<int> VecIu; // 其實可查表?
    vector<double> VecUtility;
    double CaseUtility;
    int IdxOfLastLevelIns = 0;
};
class L2_SequenceInfo
{
public:
    int sid;
    double SeqPEU = 0;
    double SeqUt = 0;
    int SeqUtCase = 0;         // 沒用到
    int SeqPEUCase = 0;        // 沒用到
    int OverMinUtilCase = 0;   // 沒用到
    int IdxOfLastLevelSeq = 0; // Position of parent node(往回找投影點扣的時候，快速定位)
    vector<L1_UtilityInfo> L1_UtInfo;
    map<int, vector<pair<int, int>>> IdxPos; // map<index of item,position of index in L2>
    // map<int, list<int>> InstanceUt;          // map<utiltiy of instance , index of instance>
};
class L3_NodeInfo
{
public:
    string pattern;
    vector<int> VecPattern;
    double SumPEU = 0;
    double SumUt = 0;
    vector<L2_SequenceInfo> L2_SeqInfo;
    int ExtensionType; // 0:I，1:S
};
vector<L3_NodeInfo> Node_SingleItem;
map<int, int> IdxNodeSingleItem; // SingleItem, Node_SingItem<>的位置

vector<int> ItemQueue; // 紀錄Externalfile中讀取Item的先後順序，來放入VecDataBase中
map<int, double> ExternalUt;
string str_EuFile = "";
string str_DBFile = "";

vector<L3_NodeInfo> PatternPath;

// vector<int> PPvalid;
// vector<L3_NodeInfo *> pp;

int Single_ItemCounter = 0;
int I_ExtensionCounter = 0;
int S_ExtensionCounter = 0;
int TotalPatternCounter = 0;
double SumSWU = 0;
double MinUtil = 0;
double memoryMB = 0;
double SumDiff = 0;

void Cout_2DVec(vector<vector<int>> vector2D)
{
    for (size_t i = 0; i < vector2D.size(); i++)
    {
        for (size_t j = 0; j < vector2D[i].size(); j++)
        {
            cout << vector2D[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;
}
void Cout_ExternalUt(map<int, double> ExternalUt)
{
    for (auto a : ExternalUt)
    {
        cout << a.first << "," << a.second << endl;
    }
}
void Cout_VecDB(vector<SeqData> VecDataBase)
{
    for (int i = 1; i < VecDataBase.size(); i++) // 0預設填空
    {
        cout << "Sid : " << VecDataBase[i].sid << endl;
        cout << "  IndexArray : ";
        for (int j = 0; j < VecDataBase[i].IndexArray.size(); j++)
        {
            cout << VecDataBase[i].IndexArray[j] << " ";
        }
        cout << endl;
        cout << " **ItemArray : ";
        for (int j = 0; j < VecDataBase[i].ItemArray.size(); j++)
        {
            cout << VecDataBase[i].ItemArray[j] << " ";
        }
        cout << endl;
        cout << "    TidArray : ";
        for (int j = 0; j < VecDataBase[i].TidArray.size(); j++)
        {
            cout << VecDataBase[i].TidArray[j] << " ";
        }
        cout << endl;
        cout << "     IuArray : ";
        for (int j = 0; j < VecDataBase[i].IuArray.size(); j++)
        {
            cout << VecDataBase[i].IuArray[j] << " ";
        }
        cout << endl;
        cout << "UtilityArray : ";
        for (int j = 0; j < VecDataBase[i].UtilityArray.size(); j++)
        {
            cout << VecDataBase[i].UtilityArray[j] << " ";
        }
        cout << endl;
        cout << "     RuArray : ";
        for (int j = 0; j < VecDataBase[i].RuArray.size(); j++)
        {
            cout << VecDataBase[i].RuArray[j] << " ";
        }
        cout << endl;
        cout << "ItemIdxTable : ";
        for (auto a : VecDataBase[i].ItemIdxTable)
        {
            cout << a.first << "[";
            for (int j = 0; j < a.second.size(); j++)
            {
                cout << a.second[j] << " ";
            }
            cout << "]  ";
        }
        cout << endl;
        cout << "      IdxTid : ";
        for (int j = 0; j < VecDataBase[i].IdxTid.size(); j++)
        {
            cout << VecDataBase[i].IdxTid[j] << " ";
        }
        cout << endl;
    }
    cout << endl;
}

void Read_ExternalUt(string ExternalFileName)
{
    string StrLine;
    ifstream read_file(ExternalFileName);
    if (!read_file.is_open())
        cout << "*** Read External Utility File FAIL. ***" << endl;
    else
    {
        while (getline(read_file, StrLine))
        {
            for (int i = 0; i < StrLine.length(); i++)
            {
                if (StrLine[i] == ':')
                {
                    string Item = StrLine.substr(0, i);
                    double Eu = stod(StrLine.substr(i + 1, StrLine.length()));

                    ExternalUt.insert(make_pair(stoi(Item), Eu));
                }
            }
        }
    }
}
void Read_Database(string DatabaseFileName)
{
    string strline;
    ifstream ifs_readDB(DatabaseFileName);
    if (!ifs_readDB.is_open())
        cout << " Read Database FAIL. " << endl;
    else
    {
        int seq = 0;
        while (getline(ifs_readDB, strline))
        {
            seq++;
            SeqData SD;
            SD.sid = seq;
            SD.IdxTid.push_back(0); // 沒有Tid 0，先填0
            SD.IdxTid.push_back(1); // Tid 1必從Index 1開始，先填1

            int IndexOfSpace = 0;
            int TidCounter = 1;
            int Index = 1;
            int ItemForUtArray = 0;
            double SU = 0;
            for (int i = strline.size() - 1; i >= 0; i--) // SU
            {
                if (strline[i] == ':')
                {
                    SU = stod((strline.substr(i + 1, strline.size() - i + 1)));
                    SumSWU += SU;
                    break;
                }
            }
            for (int i = 0; i < strline.length(); i++)
            {
                int IndexOfLeftBrackets;
                int IndexOfRightBrackets;
                if (strline[i] == '[')
                {
                    IndexOfLeftBrackets = i;
                    if (IndexOfSpace == 0) // 開頭第一個item
                    {
                        SD.ItemArray.push_back(stoi(strline.substr(0, IndexOfLeftBrackets)));
                        SD.ItemIdxTable[stoi(strline.substr(0, IndexOfLeftBrackets))].push_back(Index);

                        SD.IndexArray.push_back(Index);
                        Index++;

                        ItemForUtArray = stoi(strline.substr(0, IndexOfLeftBrackets));
                    }
                    else // 除了第一個Item以外的Item
                    {
                        SD.ItemArray.push_back(stoi(strline.substr(IndexOfSpace + 1, IndexOfLeftBrackets)));
                        SD.ItemIdxTable[stoi(strline.substr(IndexOfSpace + 1, IndexOfLeftBrackets))].push_back(Index);

                        SD.IndexArray.push_back(Index);
                        Index++;

                        ItemForUtArray = stoi(strline.substr(IndexOfSpace + 1, IndexOfLeftBrackets));
                    }
                }
                if (strline[i] == ']') // Iu
                {
                    IndexOfRightBrackets = i;
                    SD.IuArray.push_back(stod(strline.substr(IndexOfLeftBrackets + 1, IndexOfRightBrackets - IndexOfLeftBrackets - 1)));

                    SD.TidArray.push_back(TidCounter);

                    // Ru
                    for (auto a : ExternalUt)
                    {
                        if (a.first == ItemForUtArray)
                        {
                            SD.UtilityArray.push_back(a.second * stod(strline.substr(IndexOfLeftBrackets + 1, IndexOfRightBrackets - IndexOfLeftBrackets - 1))); // eu*iu
                            // 避免浮點數表示成3.10862e-015等數值(實則3.10862-0.15)
                            if (SU - (a.second * stod(strline.substr(IndexOfLeftBrackets + 1, IndexOfRightBrackets - IndexOfLeftBrackets - 1))) < 0.001) // 如果SU扣完最後一個item後的值太小
                            {
                                SD.RuArray.push_back(0);
                            }
                            else
                            {
                                SD.RuArray.push_back(SU - (a.second * stod(strline.substr(IndexOfLeftBrackets + 1, IndexOfRightBrackets - IndexOfLeftBrackets - 1))));
                            }
                            SU -= a.second * stod(strline.substr(IndexOfLeftBrackets + 1, IndexOfRightBrackets - IndexOfLeftBrackets - 1));
                        }
                    }
                }
                if (strline[i] == ' ')
                {
                    IndexOfSpace = i;
                }
                if (strline[i] == '-')
                {
                    if (strline[i + 1] == '1' && strline[i + 3] != '-') // 表示負號後第3位不是負號 -> 不是-2 -> tid++
                    {
                        SD.IdxTid.push_back(Index);
                    }
                    TidCounter++;
                }
            }
            VecDataBase.push_back(SD);
        }
    }
}

void BulidSingleItems(vector<SeqData> VecDataBase)
{
    // Node_SingleItem的順序 = ExternalUt的順序 = IdxNodeSingleItem的順序
    // 藉由IdxNodeSingleItem來更新SingleItem的資訊
    int n = 1;
    for (auto Item : ExternalUt) // 開好所有SingleItem的L3 class
    {
        if (Item.first == 0)
        {
            continue;
        }
        L3_NodeInfo L3_node;
        L3_node.pattern = to_string(Item.first);  // a
        L3_node.VecPattern.push_back(Item.first); // a
        L3_node.ExtensionType = 2;                // 2代表SingleItem，用不到，只是不想讓SingleItem的ExtensionType亂跳;
        Node_SingleItem.push_back(L3_node);       // Node_SingleItem經map排過序

        IdxNodeSingleItem.insert(make_pair(Item.first, n));
        n++;
    }

    for (int i = 1; i < VecDataBase.size(); i++)
    {
        for (int j = 0; j < VecDataBase[i].IndexArray.size(); j++)
        {
            // 第一個L2 or 不同sid的新L2
            if (!Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() ||
                Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].sid != VecDataBase[i].sid)
            {
                L2_SequenceInfo L2_seq;
                L2_seq.sid = VecDataBase[i].sid;
                L2_seq.SeqUtCase = VecDataBase[i].IndexArray[j];
                Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.push_back(L2_seq);

                L1_UtilityInfo L1_ut;
                L1_ut.VecIndex.push_back(VecDataBase[i].IndexArray[j]);
                // L1_ut.VecTid.push_back(VecDataBase[i].TidArray[j]);
                L1_ut.VecIu.push_back(VecDataBase[i].IuArray[j]);
                L1_ut.VecUtility.push_back(VecDataBase[i].UtilityArray[j]);
                L1_ut.CaseUtility = VecDataBase[i].UtilityArray[j];

                Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].L1_UtInfo.push_back(L1_ut);
                // SeqUt
                Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].SeqUt = VecDataBase[i].UtilityArray[j];
                Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].SumUt += VecDataBase[i].UtilityArray[j];

                if (VecDataBase[i].RuArray[j] > 0) // PEU=0?
                {
                    // SeqPEU
                    Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].SeqPEU = VecDataBase[i].UtilityArray[j] + VecDataBase[i].RuArray[j];
                    Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].SumPEU += VecDataBase[i].UtilityArray[j] + VecDataBase[i].RuArray[j];
                    Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].SeqPEUCase = VecDataBase[i].IndexArray[j];
                }
                // 沒用到
                if (L1_ut.CaseUtility > MinUtil)
                {
                    Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].OverMinUtilCase++;
                }
                // IdxPos
                Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].IdxPos[VecDataBase[i].IndexArray[j]].push_back({Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].L1_UtInfo.size() - 1, 0});
            }
            else // 已經存在任意L2(同一個seq但不同案例)，且sid已出現過
            {
                L1_UtilityInfo L1_ut;
                L1_ut.VecIndex.push_back(VecDataBase[i].IndexArray[j]);
                // L1_ut.VecTid.push_back(VecDataBase[i].TidArray[j]);
                L1_ut.VecIu.push_back(VecDataBase[i].IuArray[j]);
                L1_ut.VecUtility.push_back(VecDataBase[i].UtilityArray[j]);
                L1_ut.CaseUtility = VecDataBase[i].UtilityArray[j];

                Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].L1_UtInfo.push_back(L1_ut); // SeqPEU、SumPEU

                // SeqPEU、SumPEU
                if (Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].SeqPEU <
                        (VecDataBase[i].UtilityArray[j] + VecDataBase[i].RuArray[j]) &&
                    VecDataBase[i].RuArray[j] > 0)
                {
                    Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].SumPEU -= Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].SeqPEU; //(扣除)取出之前的PEU
                    Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].SumPEU += VecDataBase[i].UtilityArray[j] + VecDataBase[i].RuArray[j];                                                                                                               // 改成加現在這個更新後的PEU
                    Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].SeqPEU = VecDataBase[i].UtilityArray[j] + VecDataBase[i].RuArray[j];

                    // 沒用到
                    Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].SeqPEUCase = VecDataBase[i].IndexArray[j];
                }

                // SeqUt、SumUt
                if (Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].SeqUt <
                    VecDataBase[i].UtilityArray[j])
                {
                    Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].SumUt -= Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].SeqUt; //(扣除)取出之前的Ut
                    Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].SumUt += VecDataBase[i].UtilityArray[j];                                                                                                                                          // 改成加現在這個更新後的Ut
                    Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].SeqUt = VecDataBase[i].UtilityArray[j];

                    // 沒用到
                    Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].SeqUtCase = VecDataBase[i].IndexArray[j];
                }

                // 沒用到
                if (L1_ut.CaseUtility > MinUtil)
                {
                    Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].OverMinUtilCase++;
                }
                Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].IdxPos[VecDataBase[i].IndexArray[j]].push_back({Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo[Node_SingleItem[IdxNodeSingleItem[VecDataBase[i].ItemArray[j]]].L2_SeqInfo.size() - 1].L1_UtInfo.size() - 1, 0});
            }
        }
    }
}

void UpdateSingleItem(vector<L3_NodeInfo> &Node_SingleItem, int Item)
{
    int IdxItem = IdxNodeSingleItem[Item];
    // cout << IdxItem << endl;
    Node_SingleItem[IdxItem].SumUt = 0;
    Node_SingleItem[IdxItem].SumPEU = 0;
    for (int i = 0; i < Node_SingleItem[IdxItem].L2_SeqInfo.size(); i++)
    {
        Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqUt = 0;
        Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqPEU = 0;
        for (int j = 0; j < Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo.size(); j++)
        {
            Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIu[0] = VecDataBase[Node_SingleItem[IdxItem].L2_SeqInfo[i].sid].IuArray[Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] - 1];
            Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecUtility[0] = VecDataBase[Node_SingleItem[IdxItem].L2_SeqInfo[i].sid].UtilityArray[Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] - 1];
            Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].CaseUtility = VecDataBase[Node_SingleItem[IdxItem].L2_SeqInfo[i].sid].UtilityArray[Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] - 1];

            // SumUt、SeqUt
            if (Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].CaseUtility > Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqUt)
            {
                Node_SingleItem[IdxItem].SumUt -= Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqUt;
                Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqUt = Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].CaseUtility;
                Node_SingleItem[IdxItem].SumUt += Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqUt;

                Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqUtCase = VecDataBase[Node_SingleItem[IdxItem].L2_SeqInfo[i].sid].IndexArray[Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] - 1];
            }

            // SumPEU、SumPEU
            if (VecDataBase[Node_SingleItem[IdxItem].L2_SeqInfo[i].sid].RuArray[Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] - 1] != 0)
            {
                if (Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].CaseUtility + VecDataBase[Node_SingleItem[IdxItem].L2_SeqInfo[i].sid].RuArray[Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] - 1] > Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqPEU)
                {
                    Node_SingleItem[IdxItem].SumPEU -= Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqPEU;
                    Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqPEU = Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].CaseUtility + VecDataBase[Node_SingleItem[IdxItem].L2_SeqInfo[i].sid].RuArray[Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] - 1];
                    Node_SingleItem[IdxItem].SumPEU += Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqPEU;

                    Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqPEUCase = VecDataBase[Node_SingleItem[IdxItem].L2_SeqInfo[i].sid].IndexArray[Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] - 1];
                }
            }
            else
            {
                Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqPEU = 0;
            }
        }
    }
}

void Cout_HUSPL3(L3_NodeInfo L3_node)
{
    cout << "---------- Pattern:" << L3_node.pattern << " ----------" << endl;

    cout << "=== MinUtil:" << MinUtil << " ===" << endl;
    cout << "** SumUt:" << L3_node.SumUt << " **" << endl;
    cout << "** SumPEU:" << L3_node.SumPEU << " **" << endl;
    cout << "Extension Type:" << L3_node.ExtensionType << endl;

    for (int j = 0; j < L3_node.L2_SeqInfo.size(); j++)
    {
        cout << "Seq " << L3_node.L2_SeqInfo[j].sid << endl;
        cout << "SeqUt:" << L3_node.L2_SeqInfo[j].SeqUt << endl;
        cout << "SeqPEU:" << L3_node.L2_SeqInfo[j].SeqPEU << endl;
        cout << "SeqUtCase:" << L3_node.L2_SeqInfo[j].SeqUtCase << endl;
        cout << "SeqPEUCase:" << L3_node.L2_SeqInfo[j].SeqPEUCase << endl;
        cout << "IdxOfLastLevelSeq:" << L3_node.L2_SeqInfo[j].IdxOfLastLevelSeq << endl;
        cout << "|Idx Iu Ut|InstanceUt" << endl;
        // cout << "Over Minutil:" << L3_node.L2_SeqInfo[j].OverMinUtilCase << endl;
        for (int k = 0; k < L3_node.L2_SeqInfo[j].L1_UtInfo.size(); k++)
        {
            cout << L3_node.L2_SeqInfo[j].L1_UtInfo[k].IdxOfLastLevelIns;
            cout << "[";
            for (int h = 0; h < L3_node.L2_SeqInfo[j].L1_UtInfo[k].VecIndex.size(); h++)
            {
                cout << L3_node.L2_SeqInfo[j].L1_UtInfo[k].VecIndex[h] << " ";
            }
            cout << "]";
            cout << "[";
            for (int h = 0; h < L3_node.L2_SeqInfo[j].L1_UtInfo[k].VecIndex.size(); h++)
            {
                cout << L3_node.L2_SeqInfo[j].L1_UtInfo[k].VecIu[h] << " ";
            }
            cout << "]";
            cout << "[";
            for (int h = 0; h < L3_node.L2_SeqInfo[j].L1_UtInfo[k].VecIndex.size(); h++)
            {
                cout << L3_node.L2_SeqInfo[j].L1_UtInfo[k].VecUtility[h] << " ";
            }
            cout << "]";
            cout << L3_node.L2_SeqInfo[j].L1_UtInfo[k].CaseUtility << endl;
        }
        for (auto a : L3_node.L2_SeqInfo[j].IdxPos)
        {
            cout << "Idx:" << a.first << "->";
            for (auto pair : a.second)
            {
                cout << "(" << pair.first << "," << pair.second << ")";
            }
            cout << endl;
        }
        /*
        for (auto a : L3_node.L2_SeqInfo[j].InstanceUt)
        {
            cout << "InstanceUt:" << a.first << "->[";
            for (int InstancePos = 0; InstancePos < a.second.size(); InstancePos++)
            {
                cout << a.second[InstancePos] << " ";
            }
            cout << "]" << endl;
        }
        cout << endl;
        */
        cout << endl;
    }
    cout << endl;
}

void Cout_SeqHUSPL3(L3_NodeInfo L3_node, int seq)
{
    cout << "---------- Pattern:" << L3_node.pattern << " ----------" << endl;

    cout << "=== MinUtil:" << MinUtil << " ===" << endl;
    cout << "** SumUt:" << L3_node.SumUt << " **" << endl;
    cout << "** SumPEU:" << L3_node.SumPEU << " **" << endl;
    cout << "Extension Type:" << L3_node.ExtensionType << endl;

    cout << "Seq " << L3_node.L2_SeqInfo[seq].sid << endl;
    cout << "SeqUt:" << L3_node.L2_SeqInfo[seq].SeqUt << endl;
    cout << "SeqPEU:" << L3_node.L2_SeqInfo[seq].SeqPEU << endl;
    cout << "SeqUtCase:" << L3_node.L2_SeqInfo[seq].SeqUtCase << endl;
    cout << "SeqPEUCase:" << L3_node.L2_SeqInfo[seq].SeqPEUCase << endl;
    cout << "IdxOfLastLevelSeq:" << L3_node.L2_SeqInfo[seq].IdxOfLastLevelSeq << endl;
    cout << "|Idx Iu Ut|InstanceUt" << endl;
    // cout << "Over Minutil:" << L3_node.L2_SeqInfo[j].OverMinUtilCase << endl;
    for (int k = 0; k < L3_node.L2_SeqInfo[seq].L1_UtInfo.size(); k++)
    {
        cout << L3_node.L2_SeqInfo[seq].L1_UtInfo[k].IdxOfLastLevelIns;
        cout << "[";
        for (int h = 0; h < L3_node.L2_SeqInfo[seq].L1_UtInfo[k].VecIndex.size(); h++)
        {
            cout << L3_node.L2_SeqInfo[seq].L1_UtInfo[k].VecIndex[h] << " ";
        }
        cout << "]";
        cout << "[";
        for (int h = 0; h < L3_node.L2_SeqInfo[seq].L1_UtInfo[k].VecIndex.size(); h++)
        {
            cout << L3_node.L2_SeqInfo[seq].L1_UtInfo[k].VecIu[h] << " ";
        }
        cout << "]";
        cout << "[";
        for (int h = 0; h < L3_node.L2_SeqInfo[seq].L1_UtInfo[k].VecIndex.size(); h++)
        {
            cout << L3_node.L2_SeqInfo[seq].L1_UtInfo[k].VecUtility[h] << " ";
        }
        cout << "]";
        cout << L3_node.L2_SeqInfo[seq].L1_UtInfo[k].CaseUtility << endl;
    }
    for (auto a : L3_node.L2_SeqInfo[seq].IdxPos)
    {
        cout << "Idx:" << a.first << "->";
        for (auto pair : a.second)
        {
            cout << "(" << pair.first << "," << pair.second << ")";
        }
        cout << endl;
    }
    cout << endl;
}

void Cout_IdxNodeSingleItem(map<int, int> IdxNodeSingleItem)
{
    for (auto a : IdxNodeSingleItem)
    {
        cout << a.first << "," << a.second << endl;
    }
}

void DeleteLowRSUFromlist(L3_NodeInfo NodeUC, set<int> &ilist, set<int> &slist)
{
    map<int, double> S_RSUmap; //<Item,RSU> //<{a},RSU{a,b}=PEU{a}>
    map<int, double> I_RSUmap; //<Item,RSU>
    for (int i = 0; i < NodeUC.L2_SeqInfo.size(); i++)
    {
        // slist
        // cout << NodeUC.L2_SeqInfo[i].L1_UtInfo[0].VecIndex.back() << endl;
        // 最後一個Tid的Item不往後掃
        if (VecDataBase[NodeUC.L2_SeqInfo[i].sid].IndexArray[NodeUC.L2_SeqInfo[i].L1_UtInfo[0].VecIndex.back() - 1] < VecDataBase[NodeUC.L2_SeqInfo[i].sid].IdxTid.back())
        {
            // cout << VecDataBase[NodeUC.L2_SeqInfo[i].sid].IdxTid[VecDataBase[NodeUC.L2_SeqInfo[i].sid].TidArray[NodeUC.L2_SeqInfo[i].L1_UtInfo[0].VecIndex.back() - 1] + 1] << " ";
            for (int j = VecDataBase[NodeUC.L2_SeqInfo[i].sid].IndexArray[VecDataBase[NodeUC.L2_SeqInfo[i].sid].IdxTid[VecDataBase[NodeUC.L2_SeqInfo[i].sid].TidArray[NodeUC.L2_SeqInfo[i].L1_UtInfo[0].VecIndex.back() - 1] + 1] - 1] - 1; j < VecDataBase[NodeUC.L2_SeqInfo[i].sid].IndexArray.size(); j++)
            {
                // cout << VecDataBase[NodeUC.L2_SeqInfo[i].sid].ItemArray[j] << " ";
                S_RSUmap[VecDataBase[NodeUC.L2_SeqInfo[i].sid].ItemArray[j]] += NodeUC.L2_SeqInfo[i].SeqPEU;
                if (S_RSUmap[VecDataBase[NodeUC.L2_SeqInfo[i].sid].ItemArray[j]] >= MinUtil)
                {
                    slist.insert(VecDataBase[NodeUC.L2_SeqInfo[i].sid].ItemArray[j]);
                }
            }
        }

        // ilist
        // cout << "seq " << NodeUC.L2_SeqInfo[i].sid << endl;
        for (int j = 0; j < NodeUC.L2_SeqInfo[i].L1_UtInfo.size(); j++)
        {
            // cout << NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back() << endl;
            // cout << VecDataBase[NodeUC.L2_SeqInfo[i].sid].TidArray[NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back() - 1] << endl;
            // cout << "seq " << NodeUC.L2_SeqInfo[i].sid << endl;
            for (int k = VecDataBase[NodeUC.L2_SeqInfo[i].sid].IndexArray[NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back()] - 1; k < VecDataBase[NodeUC.L2_SeqInfo[i].sid].IndexArray.size(); k++)
            {
                if (VecDataBase[NodeUC.L2_SeqInfo[i].sid].TidArray[NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back() - 1] != VecDataBase[NodeUC.L2_SeqInfo[i].sid].TidArray[k])
                {
                    break;
                }
                // cout << " index:" << VecDataBase[NodeUC.L2_SeqInfo[i].sid].IndexArray[k] << endl;
                // cout << " extend item:" << VecDataBase[NodeUC.L2_SeqInfo[i].sid].ItemArray[k] << endl;
                // cout << "VecDataBase[" << NodeUC.L2_SeqInfo[i].sid << "].IndexArray[" << k << "]" << endl;
                // cout << "index:" << k + 1 << endl;
                // cout << NodeUC.L2_SeqInfo[i].SeqPEU << endl;
                I_RSUmap[VecDataBase[NodeUC.L2_SeqInfo[i].sid].ItemArray[k]] += NodeUC.L2_SeqInfo[i].SeqPEU;
                if (I_RSUmap[VecDataBase[NodeUC.L2_SeqInfo[i].sid].ItemArray[k]] >= MinUtil)
                {
                    ilist.insert(VecDataBase[NodeUC.L2_SeqInfo[i].sid].ItemArray[k]);
                }
            }
            // cout << endl;
        }
    }
    for (auto a : I_RSUmap)
    {
        // cout << a.first << "," << a.second << endl;
    }
}

void TraceBack(vector<L3_NodeInfo> &PatternPath, int TopNodeSeqIdx, int LastSeqIdx, int LastInsIdx, double KeepReduceUt, int PathIdx, int ExtendItemIdx)
{
    // From I-extension
    if (PatternPath[PatternPath.size() - PathIdx + 1].ExtensionType == 0)
    {
        // cout << "* " << PatternPath[PatternPath.size() - PathIdx].get().pattern << " * ";
        // cout << PatternPath[PatternPath.size() - PathIdx].get().L2_SeqInfo[LastSeqIdx].sid << endl;

        double KeepSeqUt = PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].SeqUt;
        double KeepInsUt = PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].CaseUtility;

        if (KeepInsUt > (KeepSeqUt - KeepReduceUt))
        {
            double ReduceIu = ceil(KeepReduceUt / ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]);
            // Iu夠扣
            if (PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIu.back() > ReduceIu)
            {
                /// VecDatabase ///
                VecDataBase[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].sid].UtilityArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] -= KeepInsUt - (KeepSeqUt - (ReduceIu * ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]));
                VecDataBase[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].sid].IuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] -= ReduceIu;
                for (int i = 0; i < PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1; i++)
                {
                    VecDataBase[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].sid].RuArray[i] -= KeepInsUt - (KeepSeqUt - (ReduceIu * ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]));
                }
                /// VecDatabase(end) ///

                /// PatternPath Nodes ///
                int n = TopNodeSeqIdx; // Keep住TopNode的sid來追整條path，在更新path時使用
                for (int i = PatternPath.size() - 1; i >= 0; i--)
                {
                    // Cout_SeqHUSPL3(PatternPath[i], n);
                    // cout << "--- " << PatternPath[i].get().pattern << " --- ";
                    // cout << PatternPath[i].get().L2_SeqInfo[n].sid << endl;
                    // cout << "idx=" << PatternPath[PatternPath.size() - PathIdx].get().L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() << "->";
                    for (auto L1Pos : PatternPath[i].L2_SeqInfo[n].IdxPos[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back()])
                    {
                        // cout << "(" << L1Pos.first << "," << L1Pos.second << ")" << endl;
                        // cout << PatternPath[i].get().L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecIndex[L1Pos.second] << " ";
                        // cout << PatternPath[i].get().L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecIu[L1Pos.second] << " ";
                        // cout << PatternPath[i].get().L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecUtility[L1Pos.second];
                        PatternPath[i].L2_SeqInfo[n].L1_UtInfo[L1Pos.first].CaseUtility -= KeepInsUt - (KeepSeqUt - (ReduceIu * ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]));
                        PatternPath[i].L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecUtility[L1Pos.second] -= KeepInsUt - (KeepSeqUt - (ReduceIu * ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]));
                        PatternPath[i].L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecIu[L1Pos.second] -= ReduceIu;
                    }
                    // cout << endl;

                    // Utility、PEU重算
                    PatternPath[i].SumUt -= PatternPath[i].L2_SeqInfo[n].SeqUt;
                    PatternPath[i].L2_SeqInfo[n].SeqUt = 0;
                    PatternPath[i].SumPEU -= PatternPath[i].L2_SeqInfo[n].SeqPEU;
                    PatternPath[i].L2_SeqInfo[n].SeqPEU = 0;
                    // Cout_SeqHUSPL3(PatternPath[i], n);
                    for (int j = 0; j < PatternPath[i].L2_SeqInfo[n].L1_UtInfo.size(); j++)
                    {
                        // Utility
                        if (PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility > PatternPath[i].L2_SeqInfo[n].SeqUt)
                        {
                            PatternPath[i].SumUt -= PatternPath[i].L2_SeqInfo[n].SeqUt;
                            PatternPath[i].SumUt += PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility;
                            PatternPath[i].L2_SeqInfo[n].SeqUt = PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility;
                            PatternPath[i].L2_SeqInfo[n].SeqUtCase = PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].VecIndex.back();
                        }
                        // PEU
                        // cout << "idx=" << PatternPath[PatternPath.size() - PathIdx].get().L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() << endl;
                        // cout << "ru=" << VecDataBase[PatternPath[i].get().L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].get().L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] << endl;
                        if (VecDataBase[PatternPath[i].L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] > 0)
                        {
                            if (PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility + VecDataBase[PatternPath[i].L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] > PatternPath[i].L2_SeqInfo[n].SeqPEU)
                            {
                                PatternPath[i].SumPEU -= PatternPath[i].L2_SeqInfo[n].SeqPEU;
                                PatternPath[i].SumPEU += (PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility + VecDataBase[PatternPath[i].L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1]);
                                PatternPath[i].L2_SeqInfo[n].SeqPEU = (PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility + VecDataBase[PatternPath[i].L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1]);
                                PatternPath[i].L2_SeqInfo[n].SeqPEUCase = PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].VecIndex.back();
                            }
                            else if (PatternPath[i].L2_SeqInfo[n].SeqPEU == 0)
                            {
                                PatternPath[i].L2_SeqInfo[n].SeqPEU = 0;
                            }
                        }
                    }
                    n = PatternPath[i].L2_SeqInfo[n].IdxOfLastLevelSeq;
                }
                // cout << endl;
                /// PatternPath Nodes(end) ///
            }
            else // Iu不夠扣
            {
                double KeepIu = VecDataBase[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].sid].IuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1];
                /// VecDatabase ///
                VecDataBase[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].sid].UtilityArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] -= ((KeepIu - 1) * ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]);
                VecDataBase[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].sid].IuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] = 1;
                for (int i = 0; i < PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1; i++)
                {
                    VecDataBase[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].sid].RuArray[i] -= ((KeepIu - 1) * ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]);
                }
                /// VecDatabase(end) ///

                /// PatternPath Nodes ///
                int n = TopNodeSeqIdx; // Keep住TopNode的sid來追整條path，在更新path時使用
                for (int i = PatternPath.size() - 1; i >= 0; i--)
                {
                    // Cout_SeqHUSPL3(PatternPath[i], n);
                    // cout << "--- " << PatternPath[i].get().pattern << " --- ";
                    // cout << PatternPath[i].get().L2_SeqInfo[n].sid << endl;
                    // cout << "idx=" << PatternPath[PatternPath.size() - PathIdx].get().L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() << "->";
                    for (auto L1Pos : PatternPath[i].L2_SeqInfo[n].IdxPos[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back()])
                    {
                        // cout << "(" << L1Pos.first << "," << L1Pos.second << ")" << endl;
                        // cout << PatternPath[i].get().L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecIndex[L1Pos.second] << " ";
                        // cout << PatternPath[i].get().L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecIu[L1Pos.second] << " ";
                        // cout << PatternPath[i].get().L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecUtility[L1Pos.second];
                        PatternPath[i].L2_SeqInfo[n].L1_UtInfo[L1Pos.first].CaseUtility -= ((KeepIu - 1) * ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]);
                        PatternPath[i].L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecUtility[L1Pos.second] -= ((KeepIu - 1) * ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]);
                        PatternPath[i].L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecIu[L1Pos.second] = 1;
                    }
                    // cout << endl;

                    // Utility、PEU重算
                    PatternPath[i].SumUt -= PatternPath[i].L2_SeqInfo[n].SeqUt;
                    PatternPath[i].L2_SeqInfo[n].SeqUt = 0;
                    PatternPath[i].SumPEU -= PatternPath[i].L2_SeqInfo[n].SeqPEU;
                    PatternPath[i].L2_SeqInfo[n].SeqPEU = 0;
                    // Cout_SeqHUSPL3(PatternPath[i], n);
                    for (int j = 0; j < PatternPath[i].L2_SeqInfo[n].L1_UtInfo.size(); j++)
                    {
                        // Utility
                        if (PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility > PatternPath[i].L2_SeqInfo[n].SeqUt)
                        {
                            PatternPath[i].SumUt -= PatternPath[i].L2_SeqInfo[n].SeqUt;
                            PatternPath[i].SumUt += PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility;
                            PatternPath[i].L2_SeqInfo[n].SeqUt = PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility;
                            PatternPath[i].L2_SeqInfo[n].SeqUtCase = PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].VecIndex.back();
                        }
                        // PEU
                        // cout << "idx=" << PatternPath[PatternPath.size() - PathIdx].get().L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() << endl;
                        // cout << "ru=" << VecDataBase[PatternPath[i].get().L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].get().L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] << endl;
                        if (VecDataBase[PatternPath[i].L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] > 0)
                        {
                            if (PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility + VecDataBase[PatternPath[i].L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] > PatternPath[i].L2_SeqInfo[n].SeqPEU)
                            {
                                PatternPath[i].SumPEU -= PatternPath[i].L2_SeqInfo[n].SeqPEU;
                                PatternPath[i].SumPEU += (PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility + VecDataBase[PatternPath[i].L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1]);
                                PatternPath[i].L2_SeqInfo[n].SeqPEU = (PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility + VecDataBase[PatternPath[i].L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1]);
                                PatternPath[i].L2_SeqInfo[n].SeqPEUCase = PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].VecIndex.back();
                            }
                            else if (PatternPath[i].L2_SeqInfo[n].SeqPEU == 0)
                            {
                                PatternPath[i].L2_SeqInfo[n].SeqPEU = 0;
                            }
                        }
                    }
                    n = PatternPath[i].L2_SeqInfo[n].IdxOfLastLevelSeq;
                }
                // cout << endl;
                /// PatternPath Nodes(end) ///
                KeepReduceUt -= ((KeepIu - 1) * ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]);
                TraceBack(PatternPath, TopNodeSeqIdx, PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].IdxOfLastLevelSeq, PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].IdxOfLastLevelIns, KeepReduceUt, PathIdx + 1, PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back());
            }
        }
    }

    // From S-extension
    if (PatternPath[PatternPath.size() - PathIdx + 1].ExtensionType == 1)
    {
        // cout << "* " << PatternPath[PatternPath.size() - PathIdx].get().pattern << " * ";
        // cout << PatternPath[PatternPath.size() - PathIdx].get().L2_SeqInfo[LastSeqIdx].sid << endl;

        double KeepSeqUt = PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].SeqUt;
        for (int i = PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo.size() - 1; i >= 0; i--)
        {
            double KeepInsUt = PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].CaseUtility;
            if (PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[i].VecIndex.back() < ExtendItemIdx &&
                KeepInsUt > (KeepSeqUt - KeepReduceUt))
            {
                double ReduceIu = ceil(KeepReduceUt / ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]);
                // Iu夠扣
                if (PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIu.back() > ReduceIu)
                {
                    /// VecDatabase ///
                    VecDataBase[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].sid].UtilityArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] -= KeepInsUt - (KeepSeqUt - (ReduceIu * ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]));
                    VecDataBase[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].sid].IuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] -= ReduceIu;
                    for (int i = 0; i < PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1; i++)
                    {
                        VecDataBase[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].sid].RuArray[i] -= KeepInsUt - (KeepSeqUt - (ReduceIu * ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]));
                    }
                    /// VecDatabase(end) ///

                    /// PatternPath Nodes ///
                    int n = TopNodeSeqIdx; // Keep住TopNode的sid來追整條path，在更新path時使用
                    for (int i = PatternPath.size() - 1; i >= 0; i--)
                    {
                        // Cout_SeqHUSPL3(PatternPath[i], n);
                        // cout << "--- " << PatternPath[i].get().pattern << " --- ";
                        // cout << PatternPath[i].get().L2_SeqInfo[n].sid << endl;
                        // cout << "idx=" << PatternPath[PatternPath.size() - PathIdx].get().L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() << "->";
                        for (auto L1Pos : PatternPath[i].L2_SeqInfo[n].IdxPos[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back()])
                        {
                            // cout << "(" << L1Pos.first << "," << L1Pos.second << ")" << endl;
                            // cout << PatternPath[i].get().L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecIndex[L1Pos.second] << " ";
                            // cout << PatternPath[i].get().L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecIu[L1Pos.second] << " ";
                            // cout << PatternPath[i].get().L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecUtility[L1Pos.second];
                            PatternPath[i].L2_SeqInfo[n].L1_UtInfo[L1Pos.first].CaseUtility -= KeepInsUt - (KeepSeqUt - (ReduceIu * ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]));
                            PatternPath[i].L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecUtility[L1Pos.second] -= KeepInsUt - (KeepSeqUt - (ReduceIu * ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]));
                            PatternPath[i].L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecIu[L1Pos.second] -= ReduceIu;
                        }
                        // cout << endl;

                        // Utility、PEU重算
                        PatternPath[i].SumUt -= PatternPath[i].L2_SeqInfo[n].SeqUt;
                        PatternPath[i].L2_SeqInfo[n].SeqUt = 0;
                        PatternPath[i].SumPEU -= PatternPath[i].L2_SeqInfo[n].SeqPEU;
                        PatternPath[i].L2_SeqInfo[n].SeqPEU = 0;
                        // Cout_SeqHUSPL3(PatternPath[i], n);
                        for (int j = 0; j < PatternPath[i].L2_SeqInfo[n].L1_UtInfo.size(); j++)
                        {
                            // Utility
                            if (PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility > PatternPath[i].L2_SeqInfo[n].SeqUt)
                            {
                                PatternPath[i].SumUt -= PatternPath[i].L2_SeqInfo[n].SeqUt;
                                PatternPath[i].SumUt += PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility;
                                PatternPath[i].L2_SeqInfo[n].SeqUt = PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility;
                                PatternPath[i].L2_SeqInfo[n].SeqUtCase = PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].VecIndex.back();
                            }
                            // PEU
                            // cout << "idx=" << PatternPath[PatternPath.size() - PathIdx].get().L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() << endl;
                            // cout << "ru=" << VecDataBase[PatternPath[i].get().L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].get().L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] << endl;
                            if (VecDataBase[PatternPath[i].L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] > 0)
                            {
                                if (PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility + VecDataBase[PatternPath[i].L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] > PatternPath[i].L2_SeqInfo[n].SeqPEU)
                                {
                                    PatternPath[i].SumPEU -= PatternPath[i].L2_SeqInfo[n].SeqPEU;
                                    PatternPath[i].SumPEU += (PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility + VecDataBase[PatternPath[i].L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1]);
                                    PatternPath[i].L2_SeqInfo[n].SeqPEU = (PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility + VecDataBase[PatternPath[i].L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1]);
                                    PatternPath[i].L2_SeqInfo[n].SeqPEUCase = PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].VecIndex.back();
                                }
                                else if (PatternPath[i].L2_SeqInfo[n].SeqPEU == 0)
                                {
                                    PatternPath[i].L2_SeqInfo[n].SeqPEU = 0;
                                }
                            }
                        }
                        n = PatternPath[i].L2_SeqInfo[n].IdxOfLastLevelSeq;
                    }
                    // cout << endl;
                    /// PatternPath Nodes(end) ///
                }
                else // Iu不夠扣
                {
                    if (PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIu.back() > 1)
                    {
                        double KeepIu = VecDataBase[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].sid].IuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1];
                        /// VecDatabase ///
                        VecDataBase[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].sid].UtilityArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] -= ((KeepIu - 1) * ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]);
                        VecDataBase[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].sid].IuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] = 1;
                        for (int i = 0; i < PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1; i++)
                        {
                            VecDataBase[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].sid].RuArray[i] -= ((KeepIu - 1) * ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]);
                        }
                        /// VecDatabase(end) ///

                        /// PatternPath Nodes ///
                        int n = TopNodeSeqIdx; // Keep住TopNode的sid來追整條path，在更新path時使用
                        for (int i = PatternPath.size() - 1; i >= 0; i--)
                        {
                            // Cout_SeqHUSPL3(PatternPath[i], n);
                            // cout << "--- " << PatternPath[i].get().pattern << " --- ";
                            // cout << PatternPath[i].get().L2_SeqInfo[n].sid << endl;
                            // cout << "idx=" << PatternPath[PatternPath.size() - PathIdx].get().L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() << "->";
                            for (auto L1Pos : PatternPath[i].L2_SeqInfo[n].IdxPos[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back()])
                            {
                                // cout << "(" << L1Pos.first << "," << L1Pos.second << ")" << endl;
                                // cout << PatternPath[i].get().L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecIndex[L1Pos.second] << " ";
                                // cout << PatternPath[i].get().L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecIu[L1Pos.second] << " ";
                                // cout << PatternPath[i].get().L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecUtility[L1Pos.second];
                                PatternPath[i].L2_SeqInfo[n].L1_UtInfo[L1Pos.first].CaseUtility -= ((KeepIu - 1) * ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]);
                                PatternPath[i].L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecUtility[L1Pos.second] -= ((KeepIu - 1) * ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]);
                                PatternPath[i].L2_SeqInfo[n].L1_UtInfo[L1Pos.first].VecIu[L1Pos.second] = 1;
                            }
                            // cout << endl;

                            // Utility、PEU重算
                            PatternPath[i].SumUt -= PatternPath[i].L2_SeqInfo[n].SeqUt;
                            PatternPath[i].L2_SeqInfo[n].SeqUt = 0;
                            PatternPath[i].SumPEU -= PatternPath[i].L2_SeqInfo[n].SeqPEU;
                            PatternPath[i].L2_SeqInfo[n].SeqPEU = 0;
                            // Cout_SeqHUSPL3(PatternPath[i], n);
                            for (int j = 0; j < PatternPath[i].L2_SeqInfo[n].L1_UtInfo.size(); j++)
                            {
                                // Utility
                                if (PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility > PatternPath[i].L2_SeqInfo[n].SeqUt)
                                {
                                    PatternPath[i].SumUt -= PatternPath[i].L2_SeqInfo[n].SeqUt;
                                    PatternPath[i].SumUt += PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility;
                                    PatternPath[i].L2_SeqInfo[n].SeqUt = PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility;
                                    PatternPath[i].L2_SeqInfo[n].SeqUtCase = PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].VecIndex.back();
                                }
                                // PEU
                                // cout << "idx=" << PatternPath[PatternPath.size() - PathIdx].get().L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() << endl;
                                // cout << "ru=" << VecDataBase[PatternPath[i].get().L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].get().L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] << endl;
                                if (VecDataBase[PatternPath[i].L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] > 0)
                                {
                                    if (PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility + VecDataBase[PatternPath[i].L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1] > PatternPath[i].L2_SeqInfo[n].SeqPEU)
                                    {
                                        PatternPath[i].SumPEU -= PatternPath[i].L2_SeqInfo[n].SeqPEU;
                                        PatternPath[i].SumPEU += (PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility + VecDataBase[PatternPath[i].L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1]);
                                        PatternPath[i].L2_SeqInfo[n].SeqPEU = (PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].CaseUtility + VecDataBase[PatternPath[i].L2_SeqInfo[n].sid].RuArray[PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back() - 1]);
                                        PatternPath[i].L2_SeqInfo[n].SeqPEUCase = PatternPath[i].L2_SeqInfo[n].L1_UtInfo[j].VecIndex.back();
                                    }
                                    else if (PatternPath[i].L2_SeqInfo[n].SeqPEU == 0)
                                    {
                                        PatternPath[i].L2_SeqInfo[n].SeqPEU = 0;
                                    }
                                }
                            }
                            n = PatternPath[i].L2_SeqInfo[n].IdxOfLastLevelSeq;
                        }
                        // cout << endl;
                        /// PatternPath Nodes(end) ///
                        KeepReduceUt -= ((KeepIu - 1) * ExternalUt[PatternPath[PatternPath.size() - PathIdx].VecPattern.back()]);
                    }
                    TraceBack(PatternPath, TopNodeSeqIdx, PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].IdxOfLastLevelSeq, PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].IdxOfLastLevelIns, KeepReduceUt, PathIdx + 1, PatternPath[PatternPath.size() - PathIdx].L2_SeqInfo[LastSeqIdx].L1_UtInfo[LastInsIdx].VecIndex.back());
                }
            }
        }
    }
}

// 有動到VecPattern的第一個item(也就是root)才要更新該single node

void REIHUSP_hiding(vector<L3_NodeInfo> &PatternPath)
{
    if (PatternPath.empty())
        return;
    /*
    for (int i = PatternPath.size() - 1; i >= 0; i--)
    {
        cout << PatternPath[i].pattern << endl;
    }
    cout << endl;
    */
    // 投影點處理
    double diff = PatternPath.back().SumUt - MinUtil + 1;          // diff多1避免整除使SumUt等於門檻
    double KeepSumUt = PatternPath.back().SumUt;                   // 避免前面的Instance處理後、SumUt減少影響到後續計算
    for (int i = 0; i < PatternPath.back().L2_SeqInfo.size(); i++) // Seq正序(1,2,3...)讀取
    {
        // Cout_SeqHUSPL3(PatternPath.back(), i);
        double KeepSeqUt = PatternPath.back().L2_SeqInfo[i].SeqUt;
        double ReduceUt = ceil(diff * (KeepSeqUt / KeepSumUt));
        // for (int j = PatternPath.back().get().L2_SeqInfo[i].L1_UtInfo.size() - 1; j >= 0; j--) // Instance由後往前(較後的Instacne投影點Index較大，處理後，前面的投影點Index也一併處理到了)
        for (int j = 0; j < PatternPath.back().L2_SeqInfo[i].L1_UtInfo.size(); j++) // Instance由後往前(較後的Instacne投影點Index較大，處理後，前面的投影點Index也一併處理到了)
        {
            // cout << "CaseUt:" << PatternPath.back().get().L2_SeqInfo[i].L1_UtInfo[j].CaseUtility << endl;
            double KeepReduceUt = ReduceUt;                                                             // 避免各Instance應各自減少的ReduceUt少減了
            if (PatternPath.back().L2_SeqInfo[i].L1_UtInfo[j].CaseUtility > (KeepSeqUt - KeepReduceUt)) // 可能"篡位"的Instance才須處理
            {
                // 各Instance減去的量為 : InstanceUt -=  (InstanceUt - (SeqUt - (ReduceIu * Eu)))
                double ReduceIu = ceil(ReduceUt / ExternalUt[PatternPath.back().VecPattern.back()]);
                double KeepInsUt = PatternPath.back().L2_SeqInfo[i].L1_UtInfo[j].CaseUtility;
                // 投影點Iu夠扣
                if (PatternPath.back().L2_SeqInfo[i].L1_UtInfo[j].VecIu.back() > ReduceIu)
                {
                    // Cout_SeqHUSPL3(PatternPath.back(), i);
                    /**************** VecDatabase ***************/
                    // Utility(VecDataBase)
                    VecDataBase[PatternPath.back().L2_SeqInfo[i].sid].UtilityArray[PatternPath.back().L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back() - 1] -= (KeepInsUt - (KeepSeqUt - (ReduceIu * ExternalUt[PatternPath.back().VecPattern.back()])));

                    // Iu(VecDataBase)
                    VecDataBase[PatternPath.back().L2_SeqInfo[i].sid].IuArray[PatternPath.back().L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back() - 1] -= ReduceIu;

                    // Ru(VecDataBase)
                    for (int k = 0; k < PatternPath.back().L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back() - 1; k++)
                    {
                        VecDataBase[PatternPath.back().L2_SeqInfo[i].sid].RuArray[k] -= (KeepInsUt - (KeepSeqUt - (ReduceIu * ExternalUt[PatternPath.back().VecPattern.back()])));
                    }
                    /**************** VecDatabase(end) ***************/

                    ///// PatternPath's nodes(Iu>1) /////
                    int LastSeqIdx = i;
                    for (int k = PatternPath.size() - 1; k >= 0; k--)
                    {
                        // cout << PatternPath[k].get().pattern << endl;
                        // cout << "Index:" << PatternPath.back().get().L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back() << "--->";
                        for (auto L1Pos : PatternPath[k].L2_SeqInfo[LastSeqIdx].IdxPos[PatternPath.back().L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back()])
                        {
                            // cout << "(" << L1Pos.first << "," << L1Pos.second << ")";
                            // cout << PatternPath[k].get().L2_SeqInfo[LastSeqIdx].L1_UtInfo[L1Pos.first].VecIndex[L1Pos.second] << " ";

                            //  VecUtility
                            PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[L1Pos.first].VecUtility[L1Pos.second] -= (KeepInsUt - (KeepSeqUt - (ReduceIu * ExternalUt[PatternPath.back().VecPattern.back()])));
                            // Iu
                            PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[L1Pos.first].VecIu[L1Pos.second] -= ReduceIu;
                            // Instance Utility
                            PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[L1Pos.first].CaseUtility -= (KeepInsUt - (KeepSeqUt - (ReduceIu * ExternalUt[PatternPath.back().VecPattern.back()])));
                        }

                        // Utility、PEU重新計算
                        PatternPath[k].SumUt -= PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqUt;
                        PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqUt = 0;
                        PatternPath[k].SumPEU -= PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqPEU;
                        PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqPEU = 0;
                        for (int x = 0; x < PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo.size(); x++)
                        {
                            // Ut
                            if (PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].CaseUtility > PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqUt)
                            {
                                PatternPath[k].SumUt -= PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqUt;
                                PatternPath[k].SumUt += PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].CaseUtility;
                                PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqUt = PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].CaseUtility;
                                PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqUtCase = PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].VecIndex.back();
                            }
                            // PEU
                            if (VecDataBase[PatternPath[k].L2_SeqInfo[LastSeqIdx].sid].RuArray[PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].VecIndex.back() - 1] > 0)
                            {
                                if (PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].CaseUtility + VecDataBase[PatternPath[k].L2_SeqInfo[LastSeqIdx].sid].RuArray[PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].VecIndex.back() - 1] > PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqPEU)
                                {
                                    PatternPath[k].SumPEU -= PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqPEU;
                                    PatternPath[k].SumPEU += PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].CaseUtility + VecDataBase[PatternPath[k].L2_SeqInfo[LastSeqIdx].sid].RuArray[PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].VecIndex.back() - 1];
                                    PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqPEU = PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].CaseUtility + VecDataBase[PatternPath[k].L2_SeqInfo[LastSeqIdx].sid].RuArray[PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].VecIndex.back() - 1];
                                    PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqPEUCase = PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].VecIndex.back();
                                }
                            }
                            else if (PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqPEU == 0)
                            {
                                PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqPEU = 0;
                            }
                        }
                        LastSeqIdx = PatternPath[k].L2_SeqInfo[LastSeqIdx].IdxOfLastLevelSeq;
                    }
                    ///// PatternPath's nodes(Iu>1)(end) /////
                }
                else // 投影點Iu不夠扣
                {
                    // 反之Iu=1的有更新跟沒更新一樣，不用浪費執行時間
                    if (PatternPath.back().L2_SeqInfo[i].L1_UtInfo[j].VecIu.back() > 1)
                    {
                        double KeepIu = VecDataBase[PatternPath.back().L2_SeqInfo[i].sid].IuArray[PatternPath.back().L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back() - 1];
                        /*************** VecDatabase **************/
                        // Utility(VecDataBase)
                        VecDataBase[PatternPath.back().L2_SeqInfo[i].sid].UtilityArray[PatternPath.back().L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back() - 1] -= ((KeepIu - 1) * ExternalUt[PatternPath.back().VecPattern.back()]);

                        // Iu(VecDataBase)
                        VecDataBase[PatternPath.back().L2_SeqInfo[i].sid].IuArray[PatternPath.back().L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back() - 1] = 1;

                        // Ru(VecDataBase)
                        for (int k = 0; k < PatternPath.back().L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back() - 1; k++)
                        {
                            VecDataBase[PatternPath.back().L2_SeqInfo[i].sid].RuArray[k] -= ((KeepIu - 1) * ExternalUt[PatternPath.back().VecPattern.back()]);
                        }
                        /*************** VecDatabase(end) **************/

                        ///// PatternPath's nodes(Iu<=1) /////
                        int LastSeqIdx = i;
                        for (int k = PatternPath.size() - 1; k >= 0; k--)
                        {
                            // cout << PatternPath[k].get().pattern << endl;
                            // cout << "Index:" << PatternPath.back().get().L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back() << "--->";
                            for (auto L1Pos : PatternPath[k].L2_SeqInfo[LastSeqIdx].IdxPos[PatternPath.back().L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back()])
                            {
                                // cout << "(" << L1Pos.first << "," << L1Pos.second << ")";
                                // cout << PatternPath[k].get().L2_SeqInfo[LastSeqIdx].L1_UtInfo[L1Pos.first].VecIndex[L1Pos.second] << " ";

                                //  VecUtility
                                PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[L1Pos.first].VecUtility[L1Pos.second] -= ((KeepIu - 1) * ExternalUt[PatternPath.back().VecPattern.back()]);
                                // Iu
                                PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[L1Pos.first].VecIu[L1Pos.second] = 1;
                                // Instance Utility
                                PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[L1Pos.first].CaseUtility -= ((KeepIu - 1) * ExternalUt[PatternPath.back().VecPattern.back()]);
                            }

                            // Utility、PEU重新計算
                            PatternPath[k].SumUt -= PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqUt;
                            PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqUt = 0;
                            PatternPath[k].SumPEU -= PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqPEU;
                            PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqPEU = 0;
                            for (int x = 0; x < PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo.size(); x++)
                            {
                                // Ut
                                if (PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].CaseUtility > PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqUt)
                                {
                                    PatternPath[k].SumUt -= PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqUt;
                                    PatternPath[k].SumUt += PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].CaseUtility;
                                    PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqUt = PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].CaseUtility;
                                    PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqUtCase = PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].VecIndex.back();
                                }
                                // PEU
                                if (VecDataBase[PatternPath[k].L2_SeqInfo[LastSeqIdx].sid].RuArray[PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].VecIndex.back() - 1] > 0)
                                {
                                    if (PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].CaseUtility + VecDataBase[PatternPath[k].L2_SeqInfo[LastSeqIdx].sid].RuArray[PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].VecIndex.back() - 1] > PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqPEU)
                                    {
                                        PatternPath[k].SumPEU -= PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqPEU;
                                        PatternPath[k].SumPEU += PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].CaseUtility + VecDataBase[PatternPath[k].L2_SeqInfo[LastSeqIdx].sid].RuArray[PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].VecIndex.back() - 1];
                                        PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqPEU = PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].CaseUtility + VecDataBase[PatternPath[k].L2_SeqInfo[LastSeqIdx].sid].RuArray[PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].VecIndex.back() - 1];
                                        PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqPEUCase = PatternPath[k].L2_SeqInfo[LastSeqIdx].L1_UtInfo[x].VecIndex.back();
                                    }
                                }
                                else if (PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqPEU == 0)
                                {
                                    PatternPath[k].L2_SeqInfo[LastSeqIdx].SeqPEU = 0;
                                }
                            }
                            LastSeqIdx = PatternPath[k].L2_SeqInfo[LastSeqIdx].IdxOfLastLevelSeq;
                        }
                        ///// PatternPath's nodes(Iu<=1)(end) /////

                        // ReduceUt減去投影點可以處理的量，剩餘的往下遞迴給前幾層
                        KeepReduceUt -= ((KeepIu - 1) * ExternalUt[PatternPath.back().VecPattern.back()]);
                    }

                    // 以Instance為單位做遞迴為回找
                    // PathIdx=1即PatternPath.back(),已在REIHUSP()處理完畢,從PathIdx=2開始回溯(TraceBack)
                    // cout << endl;
                    // cout << "*** " << PatternPath.back().get().pattern << " *** ";
                    // cout << PatternPath.back().get().L2_SeqInfo[i].sid << endl;

                    TraceBack(PatternPath, i, PatternPath.back().L2_SeqInfo[i].IdxOfLastLevelSeq, PatternPath.back().L2_SeqInfo[i].L1_UtInfo[j].IdxOfLastLevelIns, KeepReduceUt, 2, PatternPath.back().L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back());
                }
            }
        }
    }
}

void SingleItem_Hiding(vector<L3_NodeInfo> &Node_SingleItem, int Item)
{
    int IdxItem = IdxNodeSingleItem[Item];
    double diff = Node_SingleItem[IdxItem].SumUt - MinUtil + 1;
    double KeepSumUt = Node_SingleItem[IdxItem].SumUt;
    /*
    cout << "*** " << Node_SingleItem[IdxItem].pattern << " ***" << endl;
    cout << "SumUt=" << KeepSumUt << endl;
    cout << "diff=" << diff << endl;
    */
    for (int i = 0; i < Node_SingleItem[IdxItem].L2_SeqInfo.size(); i++)
    {
        double KeepSeqUt = Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqUt;
        double ReduceUt = ceil(diff * (KeepSeqUt / KeepSumUt));
        /*
        cout << "Seq " << Node_SingleItem[IdxItem].L2_SeqInfo[i].sid << endl;
        cout << "SeqUt:" << Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqUt << endl;
        cout << "ReduceUt:" << ReduceUt << endl;
        */
        for (int j = 0; j < Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo.size(); j++)
        {
            double ReduceUtEachIns = ReduceUt; // 下一個Instance會再讀一次原本該扣的ReduceUt，不會被之前案例影響到該Reduce的值
            if (Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].CaseUtility > (KeepSeqUt - ReduceUtEachIns))
            {
                double ReduceIu = ceil(ReduceUtEachIns / ExternalUt[Item]);
                /*
                cout << "[" << Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] << "]";
                cout << "[" << Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIu[0] << "]";
                cout << "[" << Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecUtility[0] << "]";
                cout << Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].CaseUtility << endl;
                cout << ReduceIu << endl;
                */
                if (Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIu[0] > ReduceIu)
                {
                    // Utility
                    Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecUtility[0] -= ReduceIu * ExternalUt[Item];
                    VecDataBase[Node_SingleItem[IdxItem].L2_SeqInfo[i].sid].UtilityArray[Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] - 1] -= ReduceIu * ExternalUt[Item];

                    // Iu
                    Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIu[0] -= ReduceIu;
                    VecDataBase[Node_SingleItem[IdxItem].L2_SeqInfo[i].sid].IuArray[Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] - 1] -= ReduceIu;

                    // CaseUtility
                    Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].CaseUtility -= ReduceIu * ExternalUt[Item];

                    // Ru(VecDatabase)
                    for (int k = 0; k < Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] - 1; k++)
                    {
                        // cout << VecDataBase[Node_SingleItem[IdxItem].L2_SeqInfo[i].sid].RuArray[k] << " ";
                        VecDataBase[Node_SingleItem[IdxItem].L2_SeqInfo[i].sid].RuArray[k] -= ReduceIu * ExternalUt[Item];
                    }
                    // cout << endl;

                    // SeqUt、SumUt
                    if (Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] == Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqUtCase)
                    {
                        Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqUt -= ReduceIu * ExternalUt[Item];
                        Node_SingleItem[IdxItem].SumUt -= ReduceIu * ExternalUt[Item];
                    }
                    // SeqPEU、SumPEU
                    if (Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] == Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqPEUCase)
                    {
                        Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqPEU -= ReduceIu * ExternalUt[Item];
                        Node_SingleItem[IdxItem].SumPEU -= ReduceIu * ExternalUt[Item];
                    }
                }
                else
                {
                    // Utility
                    Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecUtility[0] -= (Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIu[0] - 1) * ExternalUt[Item];
                    VecDataBase[Node_SingleItem[IdxItem].L2_SeqInfo[i].sid].UtilityArray[Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] - 1] -= (Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIu[0] - 1) * ExternalUt[Item];

                    // CaseUtility
                    Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].CaseUtility -= (Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIu[0] - 1) * ExternalUt[Item];

                    // Ru(VecDatabase)
                    for (int k = 0; k < Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] - 1; k++)
                    {
                        // cout << VecDataBase[Node_SingleItem[IdxItem].L2_SeqInfo[i].sid].RuArray[k] << " ";
                        VecDataBase[Node_SingleItem[IdxItem].L2_SeqInfo[i].sid].RuArray[k] -= (Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIu[0] - 1) * ExternalUt[Item];
                    }
                    // cout << endl;

                    // SeqUt、SumUt
                    if (Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] == Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqUtCase)
                    {
                        Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqUt -= (Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIu[0] - 1) * ExternalUt[Item];
                        Node_SingleItem[IdxItem].SumUt -= (Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIu[0] - 1) * ExternalUt[Item];
                    }
                    // SeqPEU、SumPEU
                    if (Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] == Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqPEUCase)
                    {
                        Node_SingleItem[IdxItem].L2_SeqInfo[i].SeqPEU -= (Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIu[0] - 1) * ExternalUt[Item];
                        Node_SingleItem[IdxItem].SumPEU -= (Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIu[0] - 1) * ExternalUt[Item];
                    }

                    // Iu
                    Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIu[0] = 1;
                    VecDataBase[Node_SingleItem[IdxItem].L2_SeqInfo[i].sid].IuArray[Node_SingleItem[IdxItem].L2_SeqInfo[i].L1_UtInfo[j].VecIndex[0] - 1] = 1;
                }
            }
        }
        // cout << endl;
    }
    // cout << endl;
}

void HUSP(L3_NodeInfo &NodeUC)
{
    if (NodeUC.SumPEU < MinUtil)
    {
        return;
    }
    set<int> ilist;
    set<int> slist;
    DeleteLowRSUFromlist(NodeUC, ilist, slist);

    // I-Extenesion
    for (int Iitem : ilist)
    {
        L3_NodeInfo NIF;
        NIF.SumPEU = 0;
        NIF.SumUt = 0;
        NIF.pattern = NodeUC.pattern + "-" + to_string(Iitem);
        NIF.ExtensionType = 0;
        // cout << "*** " << NIF.pattern << " ***" << endl;
        NIF.VecPattern = NodeUC.VecPattern;
        NIF.VecPattern.push_back(Iitem);

        for (int i = 0; i < NodeUC.L2_SeqInfo.size(); i++)
        {
            for (int ItemIdx : VecDataBase[NodeUC.L2_SeqInfo[i].sid].ItemIdxTable[Iitem])
            {
                for (int j = 0; j < NodeUC.L2_SeqInfo[i].L1_UtInfo.size(); j++)
                {
                    if (ItemIdx > NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back() &&
                        VecDataBase[NodeUC.L2_SeqInfo[i].sid].TidArray[ItemIdx - 1] == VecDataBase[NodeUC.L2_SeqInfo[i].sid].TidArray[NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back() - 1])
                    {
                        if (NIF.L2_SeqInfo.size() == 0 || NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].sid != VecDataBase[NodeUC.L2_SeqInfo[i].sid].sid)
                        {
                            L2_SequenceInfo SIF;
                            SIF.sid = VecDataBase[NodeUC.L2_SeqInfo[i].sid].sid;
                            SIF.IdxOfLastLevelSeq = i;
                            NIF.L2_SeqInfo.push_back(SIF);

                            L1_UtilityInfo UIF;

                            // 從上一層的哪一個L1擴展而來
                            UIF.IdxOfLastLevelIns = j;

                            // Index
                            UIF.VecIndex = NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex;
                            UIF.VecIndex.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].IndexArray[ItemIdx - 1]);
                            // Tid
                            // UIF.VecTid = NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecTid;
                            // UIF.VecTid.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].TidArray[ItemIdx - 1]);

                            // Iu、Ru、Utility(讀VecDB更新資訊)
                            for (int k = 0; k < NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex.size(); k++)
                            {
                                UIF.VecIu.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].IuArray[NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex[k] - 1]);
                                UIF.VecUtility.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].UtilityArray[NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex[k] - 1]);
                            }
                            UIF.VecIu.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].IuArray[ItemIdx - 1]);
                            UIF.VecUtility.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].UtilityArray[ItemIdx - 1]);

                            // CaseUtility(拿更新後的VecUtility來加總)
                            for (int k = 0; k < UIF.VecUtility.size(); k++)
                            {
                                UIF.CaseUtility += UIF.VecUtility[k];
                            }

                            // SeqUt、SumUt
                            NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUt = UIF.CaseUtility;
                            NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUtCase = UIF.VecIndex.back();
                            NIF.SumUt += UIF.CaseUtility;

                            // SeqPEU、SumPEU
                            if (VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1] != 0)
                            {
                                NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEU = UIF.CaseUtility + VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1];
                                NIF.SumPEU += NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEU;
                                NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEUCase = VecDataBase[NodeUC.L2_SeqInfo[i].sid].IndexArray[ItemIdx - 1];
                            }
                            else
                            {
                                NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEU = 0;
                            }

                            // Record Cases of Over MinUtil.
                            if (UIF.CaseUtility > MinUtil)
                            {
                                NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].OverMinUtilCase++;
                            }

                            NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo.push_back(UIF);
                            UIF.CaseUtility = 0;
                        }
                        else // 已存在任何L2
                        {
                            L1_UtilityInfo UIF;

                            // 從上一層的哪一個L1擴展而來
                            UIF.IdxOfLastLevelIns = j;

                            // Index
                            UIF.VecIndex = NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex;
                            UIF.VecIndex.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].IndexArray[ItemIdx - 1]);
                            // Tid
                            // UIF.VecTid = NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecTid;
                            // UIF.VecTid.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].TidArray[ItemIdx - 1]);

                            // Iu、Ru、Utility(讀VecDB更新資訊)
                            for (int k = 0; k < NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex.size(); k++)
                            {
                                UIF.VecIu.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].IuArray[NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex[k] - 1]);
                                UIF.VecUtility.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].UtilityArray[NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex[k] - 1]);
                            }
                            UIF.VecIu.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].IuArray[ItemIdx - 1]);
                            UIF.VecUtility.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].UtilityArray[ItemIdx - 1]);

                            // CaseUtility(拿更新後的VecUtility來加總)
                            for (int k = 0; k < UIF.VecUtility.size(); k++)
                            {
                                UIF.CaseUtility += UIF.VecUtility[k];
                            }

                            // 投影點的Index不同
                            if (NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo[NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo.size() - 1].VecIndex.back() != VecDataBase[NodeUC.L2_SeqInfo[i].sid].IndexArray[ItemIdx - 1])
                            {
                                // acu與SeqUt比較出SeqUt
                                if (UIF.CaseUtility > NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUt)
                                {
                                    NIF.SumUt += (UIF.CaseUtility - NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUt);
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUt = UIF.CaseUtility;
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUtCase = UIF.VecIndex.back();
                                }

                                // ru!=0 且 acu+ru比現存的SeqPEU大，比較出SeqPEU
                                if (VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1] != 0 &&
                                    UIF.CaseUtility + VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1] > NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEU)
                                {
                                    NIF.SumPEU += ((UIF.CaseUtility + VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1]) - NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEU);
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEU = UIF.CaseUtility + VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1];
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEUCase = VecDataBase[NodeUC.L2_SeqInfo[i].sid].IndexArray[ItemIdx - 1];
                                }

                                // Record Cases of Over MinUtil.
                                if (UIF.CaseUtility > MinUtil)
                                {
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].OverMinUtilCase++;
                                }

                                NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo.push_back(UIF);
                                UIF.CaseUtility = 0;
                            }
                            else // 投影點Index相同
                            {
                                // 取代之前的、Utility較小的Instance
                                if (UIF.CaseUtility > NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo[NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo.size() - 1].CaseUtility)
                                {
                                    // Record Cases of Over MinUtil.
                                    if (UIF.CaseUtility > MinUtil && NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo[NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo.size() - 1].CaseUtility <= MinUtil)
                                    {
                                        NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].OverMinUtilCase++;
                                    }

                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo.pop_back();
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo.push_back(UIF);
                                }

                                // SeqUt、SumUt
                                if (UIF.CaseUtility > NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUt)
                                {
                                    NIF.SumUt += (UIF.CaseUtility - NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUt);
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUt = UIF.CaseUtility;
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUtCase = UIF.VecIndex.back();
                                }
                                // SeqPEU、SumPEU
                                if (VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1] != 0 &&
                                    UIF.CaseUtility + VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1] > NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEU)
                                {
                                    NIF.SumPEU += ((UIF.CaseUtility + VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1]) - NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEU);
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEU = UIF.CaseUtility + VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1];
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEUCase = VecDataBase[NodeUC.L2_SeqInfo[i].sid].IndexArray[ItemIdx - 1];
                                }
                                UIF.CaseUtility = 0;
                            }
                        }
                    }
                }
            }
        }

        // IdxPos
        for (int a = 0; a < NIF.L2_SeqInfo.size(); a++)
        {
            for (int b = 0; b < NIF.L2_SeqInfo[a].L1_UtInfo.size(); b++)
            {
                // NIF.L2_SeqInfo[a].InstanceUt[NIF.L2_SeqInfo[a].L1_UtInfo[b].CaseUtility].push_back(b);
                for (int c = 0; c < NIF.L2_SeqInfo[a].L1_UtInfo[b].VecIndex.size(); c++)
                {
                    NIF.L2_SeqInfo[a].IdxPos[NIF.L2_SeqInfo[a].L1_UtInfo[b].VecIndex[c]].push_back({b, c});
                }
            }
        }

        // PatternPath
        if (PatternPath.empty())
        {
            PatternPath.push_back(ref(NodeUC));
        }
        else
        {
            while (!PatternPath.empty() &&
                   NIF.VecPattern.size() <= PatternPath.back().VecPattern.size())
            {
                PatternPath.pop_back();
            }
            if (PatternPath.empty() ||
                NodeUC.VecPattern.size() > PatternPath.back().VecPattern.size())
            {
                PatternPath.push_back(ref(NodeUC));
            }
        }
        PatternPath.push_back(ref(NIF));

        if (NIF.SumUt >= MinUtil)
        {
            I_ExtensionCounter++;
            // cout << "pattern " << NIF.pattern << " start REIHUSP_hiding() ..." << endl;
            REIHUSP_hiding(PatternPath);
            // test(PatternPath);
            // SumDiff += (MinUtil - NIF.SumUt);
        }
        if (NIF.VecPattern.size() <= 3)
        {
            // Cout_HUSPL3(NIF);
        }

        HUSP(NIF);
    }

    // S-Extension
    for (int Sitem : slist)
    {
        L3_NodeInfo NIF;
        NIF.SumPEU = 0;
        NIF.SumUt = 0;
        NIF.pattern = NodeUC.pattern + "," + to_string(Sitem);
        NIF.ExtensionType = 1;
        // cout << "*** " << NIF.pattern << " ***" << endl;

        NIF.VecPattern = NodeUC.VecPattern;
        NIF.VecPattern.push_back(Sitem);

        for (int i = 0; i < NodeUC.L2_SeqInfo.size(); i++)
        {
            for (int ItemIdx : VecDataBase[NodeUC.L2_SeqInfo[i].sid].ItemIdxTable[Sitem])
            {
                for (int j = 0; j < NodeUC.L2_SeqInfo[i].L1_UtInfo.size(); j++)
                {
                    if (ItemIdx > NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back() &&
                        VecDataBase[NodeUC.L2_SeqInfo[i].sid].TidArray[ItemIdx - 1] > VecDataBase[NodeUC.L2_SeqInfo[i].sid].TidArray[NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex.back() - 1])
                    {
                        // 首個L2
                        if (NIF.L2_SeqInfo.size() == 0 || NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].sid != VecDataBase[NodeUC.L2_SeqInfo[i].sid].sid)
                        {
                            L2_SequenceInfo SIF;
                            SIF.sid = VecDataBase[NodeUC.L2_SeqInfo[i].sid].sid;
                            SIF.IdxOfLastLevelSeq = i;
                            NIF.L2_SeqInfo.push_back(SIF);

                            L1_UtilityInfo UIF;

                            // 從上一層的哪一個L1擴展而來
                            UIF.IdxOfLastLevelIns = j;

                            // Index
                            UIF.VecIndex = NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex;
                            UIF.VecIndex.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].IndexArray[ItemIdx - 1]);
                            // IdxPos
                            for (int k = 0; k < UIF.VecIndex.size(); k++)
                            {
                                // NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].IdxPos[UIF.VecIndex[k]].push_back({NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo.size(), k});
                            }
                            // Tid
                            // UIF.VecTid = NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecTid;
                            // UIF.VecTid.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].TidArray[ItemIdx - 1]);

                            // Iu、Utility(讀VecDB更新資訊)
                            for (int k = 0; k < NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex.size(); k++)
                            {
                                UIF.VecIu.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].IuArray[NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex[k] - 1]);
                                UIF.VecUtility.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].UtilityArray[NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex[k] - 1]);
                            }
                            UIF.VecIu.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].IuArray[ItemIdx - 1]);
                            UIF.VecUtility.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].UtilityArray[ItemIdx - 1]);

                            // CaseUtility(拿更新後的VecUtility來加總)
                            for (int k = 0; k < UIF.VecUtility.size(); k++)
                            {
                                UIF.CaseUtility += UIF.VecUtility[k];
                            }

                            // SeqUt、SumUt
                            NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUt = UIF.CaseUtility;
                            NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUtCase = UIF.VecIndex.back();
                            NIF.SumUt += UIF.CaseUtility;

                            // SeqPEU、SumPEU
                            if (VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1] != 0)
                            {
                                NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEU = UIF.CaseUtility + VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1];
                                NIF.SumPEU += NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEU;
                                NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEUCase = VecDataBase[NodeUC.L2_SeqInfo[i].sid].IndexArray[ItemIdx - 1];
                            }
                            else
                            {
                                NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEU = 0;
                            }

                            // Record Cases of Over MinUtil.
                            if (UIF.CaseUtility > MinUtil)
                            {
                                NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].OverMinUtilCase++;
                            }

                            NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo.push_back(UIF);
                            UIF.CaseUtility = 0;
                        }
                        else // 已存在任何L2
                        {
                            L1_UtilityInfo UIF;

                            // 從上一層的哪一個L1擴展而來
                            UIF.IdxOfLastLevelIns = j;

                            // Index
                            UIF.VecIndex = NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex;
                            UIF.VecIndex.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].IndexArray[ItemIdx - 1]);
                            // Tid
                            // UIF.VecTid = NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecTid;
                            // UIF.VecTid.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].TidArray[ItemIdx - 1]);

                            // Iu、Ru、Utility(讀VecDB更新資訊)
                            for (int k = 0; k < NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex.size(); k++)
                            {
                                UIF.VecIu.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].IuArray[NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex[k] - 1]);
                                UIF.VecUtility.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].UtilityArray[NodeUC.L2_SeqInfo[i].L1_UtInfo[j].VecIndex[k] - 1]);
                            }
                            UIF.VecIu.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].IuArray[ItemIdx - 1]);
                            UIF.VecUtility.push_back(VecDataBase[NodeUC.L2_SeqInfo[i].sid].UtilityArray[ItemIdx - 1]);

                            // CaseUtility(拿更新後的VecUtility來加總)
                            for (int k = 0; k < UIF.VecUtility.size(); k++)
                            {
                                UIF.CaseUtility += UIF.VecUtility[k];
                            }

                            // 投影點的Index不同
                            if (NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo[NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo.size() - 1].VecIndex.back() != VecDataBase[NodeUC.L2_SeqInfo[i].sid].IndexArray[ItemIdx - 1])
                            {
                                // acu與SeqUt比較出SeqUt
                                if (UIF.CaseUtility > NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUt)
                                {
                                    NIF.SumUt += (UIF.CaseUtility - NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUt);
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUt = UIF.CaseUtility;
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUtCase = UIF.VecIndex.back();
                                }

                                // ru!=0 且 acu+ru比現存的SeqPEU大，比較出SeqPEU
                                if (VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1] != 0 &&
                                    UIF.CaseUtility + VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1] > NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEU)
                                {
                                    NIF.SumPEU += ((UIF.CaseUtility + VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1]) - NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEU);
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEU = UIF.CaseUtility + VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1];
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEUCase = VecDataBase[NodeUC.L2_SeqInfo[i].sid].IndexArray[ItemIdx - 1];
                                }

                                // Record Cases of Over MinUtil.
                                if (UIF.CaseUtility > MinUtil)
                                {
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].OverMinUtilCase++;
                                }

                                NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo.push_back(UIF);
                            }
                            else // 投影點Index相同
                            {
                                if (UIF.CaseUtility > NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo[NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo.size() - 1].CaseUtility)
                                {
                                    // Record Cases of Over MinUtil.
                                    if (UIF.CaseUtility > MinUtil && NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo[NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo.size() - 1].CaseUtility <= MinUtil)
                                    {
                                        NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].OverMinUtilCase++;
                                    }
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo.pop_back();
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].L1_UtInfo.push_back(UIF);
                                }
                                // SeqUt、SumUt
                                if (UIF.CaseUtility > NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUt)
                                {
                                    NIF.SumUt += (UIF.CaseUtility - NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUt);
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUt = UIF.CaseUtility;
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqUtCase = UIF.VecIndex.back(); // Record Cases of Over MinUtil.
                                }
                                // SeqPEU、SumPEU
                                if (VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1] != 0 &&
                                    UIF.CaseUtility + VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1] > NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEU)
                                {
                                    NIF.SumPEU += ((UIF.CaseUtility + VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1]) - NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEU);
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEU = UIF.CaseUtility + VecDataBase[NodeUC.L2_SeqInfo[i].sid].RuArray[ItemIdx - 1];
                                    NIF.L2_SeqInfo[NIF.L2_SeqInfo.size() - 1].SeqPEUCase = VecDataBase[NodeUC.L2_SeqInfo[i].sid].IndexArray[ItemIdx - 1];
                                }
                            }
                            UIF.CaseUtility = 0;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        // IdxPos
        for (int a = 0; a < NIF.L2_SeqInfo.size(); a++)
        {
            for (int b = 0; b < NIF.L2_SeqInfo[a].L1_UtInfo.size(); b++)
            {
                // NIF.L2_SeqInfo[a].InstanceUt[NIF.L2_SeqInfo[a].L1_UtInfo[b].CaseUtility].push_back(b);
                for (int c = 0; c < NIF.L2_SeqInfo[a].L1_UtInfo[b].VecIndex.size(); c++)
                {
                    NIF.L2_SeqInfo[a].IdxPos[NIF.L2_SeqInfo[a].L1_UtInfo[b].VecIndex[c]].push_back({b, c});
                }
            }
        }

        // PatternPath
        if (PatternPath.empty())
        {
            PatternPath.push_back(ref(NodeUC));
        }
        else
        {
            while (!PatternPath.empty() &&
                   NIF.VecPattern.size() <= PatternPath.back().VecPattern.size())
            {
                PatternPath.pop_back();
            }
            if (PatternPath.empty() ||
                NodeUC.VecPattern.size() > PatternPath.back().VecPattern.size())
            {
                PatternPath.push_back(ref(NodeUC));
            }
        }
        PatternPath.push_back(ref(NIF));

        if (NIF.SumUt >= MinUtil)
        {
            S_ExtensionCounter++;
            // cout << "pattern " << NIF.pattern << " start REIHUSP_hiding() ..." << endl;
            REIHUSP_hiding(PatternPath);
            // test(PatternPath);
            // SumDiff += (MinUtil - NIF.SumUt);
        }
        if (NIF.VecPattern.size() <= 3)
        {
            // Cout_HUSPL3(NIF);
        }
        HUSP(NIF);
    }
}

void DBWriteBack(vector<SeqData> VecDataBase)
{
    string Filename = "Output_" + str_DBFile + "_Minutil_";
    Filename.append(to_string((int)MinUtil).append(".txt"));
    ofstream writeFile(Filename);
    if (writeFile.is_open())
    {
        for (int i = 1; i < VecDataBase.size(); i++)
        {
            int Tid = 1;
            for (int j = 0; j < VecDataBase[i].IndexArray.size(); j++)
            {
                if (VecDataBase[i].TidArray[j] != Tid)
                {
                    writeFile << "-1 ";
                    Tid++;
                }
                writeFile << VecDataBase[i].ItemArray[j];
                writeFile << "[";
                writeFile << VecDataBase[i].IuArray[j];
                writeFile << "] ";
            }
            writeFile << "-1 -2  SUtility:";
            writeFile << VecDataBase[i].RuArray[0] + VecDataBase[i].UtilityArray[0] << "\n";
        }
    }
    else
    {
        return;
    }
}

void OutputResult(double time)
{
    string Filename = "Output_" + str_DBFile + "_Experiment_Minutil_";
    Filename.append(to_string((int)MinUtil).append(".txt"));
    ofstream writeFile(Filename);
    if (writeFile.is_open())
    {
        writeFile << "Database:" << str_DBFile << "\n";
        writeFile << "=====================" << "\n";
        writeFile << "Single item Counter : " << Single_ItemCounter << "\n";
        writeFile << "I_Extension Counter : " << I_ExtensionCounter << "\n";
        writeFile << "S_Extension Counter : " << S_ExtensionCounter << "\n";
        writeFile << "Total pattern Counter : " << Single_ItemCounter + I_ExtensionCounter + S_ExtensionCounter << "\n";
        writeFile << "Time consumption:" << time << "s" << "\n";
        writeFile << "Memory Usage:" << memoryMB << "(MB)" << "\n";
        writeFile << "=====================" << "\n";
    }
    else
    {
        return;
    }
}

int main()
{
    // cout << __cplusplus << endl; //C++ version
    SeqData SD;
    SD.sid = 0;
    VecDataBase.push_back(SD); // VecDataBase[0]留空，item從1開始記錄

    L3_NodeInfo EmptyL3;
    Node_SingleItem.push_back(EmptyL3);

    vector<int> v(1, 0);
    ExternalUt.insert(make_pair(0, 0));
    cout << endl;

    //str_EuFile = "simple_utb.txt";
    //str_DBFile = "simple_db.txt";

    
    str_EuFile = "05.foodmart_ExternalUtility.txt";
    str_DBFile = "05.foodmart.txt";
    
    
    //str_EuFile = "4_sign_ExternalUtility.txt";
    //str_DBFile = "4_sign.txt";
    
    MinUtil = 2752;
    cout << "*** (Hiding)Min utility = " << MinUtil << " ***" << endl;
    cout << "*** (Hiding)Database : " << str_DBFile << " ***" << endl;
    cout << "*** (Hiding)Eu : " << str_EuFile << " ***" << endl;
    // cout << "******** Hiding HUSP process ********" << endl;
    // cout << "Start Reading External Utility..." << endl;
    Read_ExternalUt(str_EuFile);
    // cout << "Start Reading Database..." << endl;
    Read_Database(str_DBFile);

    // cout << "Start Building Single Items..." << endl;
    BulidSingleItems(VecDataBase);

    // Cout_VecDB(VecDataBase);
    //  Cout_ExternalUt(ExternalUt);
    //  cout << endl;
    //  Cout_IdxNodeSingleItem(IdxNodeSingleItem);

    double Threshold = 1;
    // double MinUtil = SumSWU * Threshold;
    // double MinUtil = 0;
    // cout << MinUtil << "=" << SumSWU << "*" << Threshold << endl;

    double time = 0;
    clock_t start, end;
    start = clock();
    // Node_SingleItem.size()
    for (int i = 1; i < Node_SingleItem.size(); i++)
    {
        // Cout_HUSPL3(Node_SingleItem[i]);
        //   cout << "Start Update SingleItem info " << Node_SingleItem[i].pattern << " ..." << endl;
        UpdateSingleItem(Node_SingleItem, stoi(Node_SingleItem[i].pattern)); // 重讀一次Single Itme在DB的資訊(因為被其他Node改到)
        if (Node_SingleItem[i].SumUt >= MinUtil)
        {
            Single_ItemCounter++;
            // cout << "Start Reduce Single item " << Node_SingleItem[i].pattern << " ..." << endl;
            SingleItem_Hiding(Node_SingleItem, i); // Update()後的SingleItem依然大於門檻
            // SumDiff += (MinUtil - Node_SingleItem[i].SumUt);
        }
        PatternPath.push_back(ref(Node_SingleItem[i]));
        // cout << "Start HUSP of " << Node_SingleItem[i].pattern << " ..." << endl;
        HUSP(Node_SingleItem[i]);
        PatternPath.clear();
    }
    //Cout_VecDB(VecDataBase);

    // $LASTEXITCODE
    end = clock();

    cout << endl;
    cout << "SWU : " << SumSWU << endl;
    cout << "Threshold : " << Threshold << endl;
    cout << "Database:" << str_DBFile << endl;
    cout << "*** MinUtil = " << MinUtil << " ***" << endl;
    cout << "Single item Counter : " << Single_ItemCounter << endl;
    cout << "I_Extension Counter : " << I_ExtensionCounter << endl;
    cout << "S_Extension Counter : " << S_ExtensionCounter << endl;
    cout << "Total pattern Counter : " << Single_ItemCounter + I_ExtensionCounter + S_ExtensionCounter << endl;
    // cout << "Sum of reduced utility : " << SumDiff << endl;
    cout << endl;
    time = (double)(end - start) / CLOCKS_PER_SEC;
    
    // Memory Usage
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
    {
        memoryMB = pmc.WorkingSetSize / (1024.0 * 1024.0); // 轉成 MB
        cout << "Memory Usage (Working Set Size): " << memoryMB << " MB" << endl;
    }
    /////////////////////

    cout << "Time consumption:" << time << "s" << endl;
    cout << endl;

    cout << "Start Write out sanitized dataset ..." << endl;
    DBWriteBack(VecDataBase);

    cout << "Start Write out hiding experiment ..." << endl;
    OutputResult(time);
    cout << endl;

    // cout << "Start Write out HUS patterns ..." << endl;
    // WriteOutHUSPattern(HUSPattern, time);
    
    cout << "******** Hiding HUSP process END ********" << endl;
    cout << endl;
    return 0;
}