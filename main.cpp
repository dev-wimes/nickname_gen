#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <codecvt>
#include <locale>
#include <iomanip>

using namespace std;

static inline bool isVowel(char c) {
    return c=='a'||c=='e'||c=='i'||c=='o'||c=='u'||c=='y';
}
static inline bool isConsonant(char c) { return !isVowel(c); }

// UTF-8(string) -> UTF-16(u16string)
static u16string utf8ToUtf16(const string& s) {
    wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> conv;
    return conv.from_bytes(s);
}

// UTF-16 code unit들을 big-endian 바이트로 쪼개서 a~z로 매핑
static vector<char> extractLettersFromU16(const u16string& name16) {
    vector<char> letters;
    letters.reserve(name16.size() * 2);

    for (char16_t cu : name16) {
        unsigned high = (cu >> 8) & 0xFF;
        unsigned low  = (cu >> 0) & 0xFF;

        letters.push_back(char((high % 26) + 97));
        letters.push_back(char((low  % 26) + 97));
    }
    return letters;
}

// 규칙 체크
static bool canAppend(char lastChar,
                      int repeatCount,          // lastChar 연속 횟수(1~2)
                      bool lastWasConsonant,
                      char nextChar) {
    // 규칙 5: 같은 글자 3연속 금지
    if (nextChar == lastChar && repeatCount >= 2) return false;

    // 규칙 4: 자음 연속은 같은 자음만 허용(tt, ss)
    // 규칙 6: 자음 + h는 wildcard로 허용(bh, ch)
    //         단 bbh, tth는 불가능 -> 이미 double consonant(repeatCount>=2)면 h 금지
    if (lastWasConsonant && isConsonant(nextChar)) {
        if (nextChar == lastChar) return true; // 같은 자음 연속 OK
        if (nextChar == 'h') {
            if (repeatCount >= 2) return false; // bbh/tth 방지
            return true;
        }
        return false; // 다른 자음 연속 금지
    }

    // 모음 연속은 허용(규칙 3)
    return true;
}

static void dfsGenerate(const vector<char>& letters,
                        int targetLen,
                        int pos,
                        string& cur,
                        char lastChar,
                        int repeatCount,
                        bool lastWasConsonant,
                        int vowelCount,
                        ofstream& out) {
    if (pos == targetLen) {
        // (권장) 결과 닉네임도 모음 2개 이상만 출력
        if (vowelCount >= 2) {
            cout << cur << "\n";
            out  << cur << "\n";
        }
        return;
    }

    for (char c : letters) {
        if (pos > 0) {
            if (!canAppend(lastChar, repeatCount, lastWasConsonant, c)) continue;
        }

        int nextRepeat = 1;
        if (pos > 0 && c == lastChar) nextRepeat = repeatCount + 1;

        bool nextWasConsonant = isConsonant(c);
        int nextVowelCount = vowelCount + (isVowel(c) ? 1 : 0);

        cur.push_back(c);
        dfsGenerate(letters, targetLen, pos + 1, cur,
                    c, nextRepeat, nextWasConsonant, nextVowelCount, out);
        cur.pop_back();
    }
}


static void printU16UnitsHex(const u16string& name16) {
    cout << "[UTF-16 code units]\n";
    for (char16_t cu : name16) {
        cout << "0x" << hex << uppercase << setw(4) << setfill('0')
             << (unsigned)cu << " ";
    }
    cout << dec << "\n\n";
}

static void printBytesAndLetters(const u16string& name16, const vector<char>& letters) {
    cout << "[Split bytes]\n";
    for (char16_t cu : name16) {
        unsigned high = (cu >> 8) & 0xFF;
        unsigned low  = (cu >> 0) & 0xFF;
        cout << "0x" << hex << uppercase << setw(2) << setfill('0') << high << " ";
        cout << "0x" << hex << uppercase << setw(2) << setfill('0') << low  << " ";
    }
    cout << dec << "\n\n";

    cout << "[letters a~z]\n";
    for (char c : letters) cout << c << " ";
    cout << "\n\n";
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cout << "이름을 입력하세요(예: 홍길동): ";
    string nameUtf8;
    getline(cin, nameUtf8);

    if (nameUtf8.empty()) {
        cerr << "입력이 비었습니다.\n";
        return 1;
    }

    u16string name16;
    try {
        name16 = utf8ToUtf16(nameUtf8);
    } catch (const range_error& e) {
        cerr << "UTF-8 -> UTF-16 변환 실패: " << e.what() << "\n";
        return 1;
    }

    // 알파벳 추출
    vector<char> letters = extractLettersFromU16(name16);

    printU16UnitsHex(name16);
    printBytesAndLetters(name16, letters);
    
    // 모음이 2개 미만이면 추가 입력
    int vowelCnt = 0;
    for (char c : letters) if (isVowel(c)) vowelCnt++;

    while (vowelCnt < 2) {
        cout << "add vowel (a/e/i/o/u/y): ";
        char v;
        cin >> v;
        if (isVowel(v)) {
            letters.push_back(v);
            vowelCnt++;
        } else {
            cout << "plz input vowel\n";
        }
    }

    int L = 0;
    while (true) {
        cout << "nickname length?(5~7): ";
        cin >> L;
        if (L >= 5 && L <= 7) break;
        cout << "invalid input\n";
    }

    ofstream out("nick.txt");
    if (!out.is_open()) {
        cerr << "nick.txt 열기 실패\n";
        return 1;
    }

    string cur;
    cur.reserve(L);

    dfsGenerate(letters, L, 0, cur, '\0', 0, false, 0, out);
    return 0;
}