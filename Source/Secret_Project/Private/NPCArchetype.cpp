#include "NPCArchetype.h"
#include "ANPCCharacter.h"

// ── 성격 기반 대사 생성 (현대 배경) ───────────────────────
// 프로필에 명시 대사가 없을 때 성격 톤으로 자동 생성 → 최소 입력으로 캐릭터가 말하게.
// 같은 성격을 공유하는 NPC가 똑같이 말하지 않도록, 이름 해시로 풀에서 다른 구간을 골라준다(양산 다양성).

static TArray<FString> PickLines(const TArray<FString>& Pool, const FString& Name, int32 Count)
{
    TArray<FString> Out;
    if (Pool.Num() == 0) return Out;
    const int32 Start = static_cast<int32>(GetTypeHash(Name) % static_cast<uint32>(Pool.Num()));
    const int32 N = FMath::Min(Count, Pool.Num());
    for (int32 i = 0; i < N; ++i)
        Out.Add(Pool[(Start + i) % Pool.Num()]);
    return Out;
}

TArray<FString> UNPCArchetypeLibrary::GenerateDialogue(ENPCPersonality P, const FString& Name)
{
    switch (P)
    {
    case ENPCPersonality::Cheerful:
        return PickLines({ TEXT("어! 안녕, 오늘도 컨디션 좋아 보인다?"),
                           TEXT("점심 뭐 먹을지 정했어? 같이 갈래?"),
                           TEXT("주말에 시내 새로 생긴 데 가봤어? 완전 핫해!"),
                           TEXT("하하, 또 보네! 반가워 진짜."),
                           TEXT("야, 이따 끝나고 노래방 콜?"),
                           TEXT("뭔가 좋은 일 있어 보이는데~ 말해봐!"),
                           TEXT("오늘 날씨 미쳤다, 그냥 놀고 싶어!"),
                           TEXT("표정 왜 그래~ 무슨 일 있으면 나한테 말해!"),
                           TEXT("나 방금 진짜 웃긴 거 봤는데, 들어볼래?"),
                           TEXT("힘들 땐 일단 좀 웃자! 그럼 반은 풀려, 진짜로."),
                           TEXT("우리 또 보네? 이쯤 되면 인연이다 인연!") }, Name, 4);
    case ENPCPersonality::Shy:
        return PickLines({ TEXT("아... 안녕하세요."),
                           TEXT("저, 저한테 볼일이... 있으세요?"),
                           TEXT("그, 그럼... 조심히 가세요."),
                           TEXT("사람 많은 데는 좀... 그래서요."),
                           TEXT("책 읽는 게... 제일 편해요."),
                           TEXT("어, 음... 딱히 할 말은 없는데..."),
                           TEXT("저… 말 걸어줘서, 고마워요. 좀 외로웠거든요."),
                           TEXT("가끔은… 누가 곁에 있는 것만으로도 안심돼요."),
                           TEXT("그… 천천히 말해도 될까요? 생각 정리가 좀 느려서."),
                           TEXT("오늘은… 용기 내서 밖에 나와봤어요.") }, Name, 4);
    case ENPCPersonality::Intellectual:
        return PickLines({ TEXT("흥미롭군. 뭘 알고 싶지?"),
                           TEXT("정보는 곧 힘이야. 잘 새겨둬."),
                           TEXT("질문이 좋네. 답을 찾는 자세가 중요하지."),
                           TEXT("요즘 읽는 자료가 꽤 재밌어. 추천해줄까?"),
                           TEXT("논리적으로 생각해 봐. 답은 거기 있어."),
                           TEXT("데이터를 보면 답이 보이는 법이지."),
                           TEXT("성급한 결론은 금물이야. 근거를 모아야지."),
                           TEXT("세상엔 아직 설명 안 되는 일이 많아. …요즘 특히."),
                           TEXT("기록해 두는 습관, 의외로 너를 구해줄 거야."),
                           TEXT("궁금한 게 있으면 물어봐. 아는 건 답해줄게.") }, Name, 4);
    case ENPCPersonality::Tough:
        return PickLines({ TEXT("뭘 봐? 용건 있으면 빨리 말해."),
                           TEXT("어설픈 건 질색이야."),
                           TEXT("흥, 그럭저럭 봐줄 만하군."),
                           TEXT("훈련 안 빠지고 나오는 건 인정한다."),
                           TEXT("약한 소리 할 거면 저리 가."),
                           TEXT("덤빌 거면 제대로 덤벼."),
                           TEXT("쫄지 마. 쫄면 지는 거야, 뭐든."),
                           TEXT("…뭐, 도움 필요하면 말은 해. 모른 척은 안 한다."),
                           TEXT("강해지고 싶으면 발부터 움직여. 머리는 나중이고."),
                           TEXT("징징댈 시간에 한 번 더 부딪혀보는 게 낫지.") }, Name, 4);
    case ENPCPersonality::Kind:
        return PickLines({ TEXT("어서 와요. 어디 아픈 덴 없죠?"),
                           TEXT("무리하지 말아요, 알았죠?"),
                           TEXT("언제든 쉬어 가도 괜찮아요."),
                           TEXT("밥은 챙겨 먹고 다녀요?"),
                           TEXT("힘든 일 있으면 언제든 얘기해요."),
                           TEXT("따뜻한 거 한 잔 줄까요?"),
                           TEXT("얼굴이 좀 피곤해 보여요. 잠은 잘 자요?"),
                           TEXT("괜찮아요, 천천히 해도 돼요. 내가 기다릴게요."),
                           TEXT("혼자 다 짊어지려 하지 말아요. 나눠 들면 가벼워져요."),
                           TEXT("오늘도 잘 해냈어요. 그거면 충분해요.") }, Name, 4);
    case ENPCPersonality::Cool:
        return PickLines({ TEXT("……용건은?"),
                           TEXT("쓸데없는 얘긴 사양하지."),
                           TEXT("그래. 이만 가도 되겠어?"),
                           TEXT("시간 낭비는 싫어해."),
                           TEXT("……뭐, 알아서 해."),
                           TEXT("필요한 것만 말해."),
                           TEXT("……너, 생각보단 끈질기군. 나쁘진 않아."),
                           TEXT("감상은 됐고. 결론부터 말해."),
                           TEXT("……조심해. 요즘 거리가 평소 같지 않으니까."),
                           TEXT("말 많은 건 질색이지만… 네 말은, 들어줄 만하군.") }, Name, 4);
    default:
        return { FString::Printf(TEXT("%s: ...안녕."), *Name) };
    }
}

TArray<FString> UNPCArchetypeLibrary::GenerateEveningDialogue(ENPCPersonality P, const FString& Name)
{
    switch (P)
    {
    case ENPCPersonality::Cheerful:
        return PickLines({ TEXT("밤공기 좋다! 아직 안 들어가?"), TEXT("이 시간 거리도 분위기 있지 않냐?"),
                           TEXT("야식 땡기는데, 같이 갈 사람~?") }, Name, 2);
    case ENPCPersonality::Shy:
        return PickLines({ TEXT("밤엔... 좀 무섭지 않아요?"), TEXT("어두우니까... 조심해서 가요."),
                           TEXT("이 시간엔 사람도 없고... 좋네요.") }, Name, 2);
    case ENPCPersonality::Intellectual:
        return PickLines({ TEXT("밤은 사색하기 좋은 시간이지."), TEXT("야경 보면 머리가 맑아져."),
                           TEXT("이런 밤엔 생각이 잘 정리돼.") }, Name, 2);
    case ENPCPersonality::Tough:
        return PickLines({ TEXT("이 시간에 돌아다니다니, 간이 크군."), TEXT("밤거리는 위험해. 알아서 해."),
                           TEXT("늦었으니 빨리 들어가.") }, Name, 2);
    case ENPCPersonality::Kind:
        return PickLines({ TEXT("늦었네요. 따뜻하게 입고 다녀요."), TEXT("밤길 조심하고, 푹 쉬어요."),
                           TEXT("집까지 잘 들어가요, 꼭.") }, Name, 2);
    case ENPCPersonality::Cool:
        return PickLines({ TEXT("밤이군. ……그래서?"), TEXT("이런 시간에 무슨 일이지."),
                           TEXT("……밤이 더 조용해서 낫지.") }, Name, 2);
    default:
        return {};
    }
}

TArray<FString> UNPCArchetypeLibrary::GenerateBondDialogue(ENPCPersonality P, const FString& Name)
{
    switch (P)
    {
    case ENPCPersonality::Cheerful:
        return PickLines({ TEXT("너랑 있으면 진짜 즐거워! 우리 친구지?"), TEXT("뭐든 같이 하자, 응?"),
                           TEXT("넌 진짜 내 베프야!") }, Name, 2);
    case ENPCPersonality::Shy:
        return PickLines({ TEXT("이제... 당신이라면 편하게 얘기할 수 있어요."), TEXT("저, 믿고 있어요... 당신을."),
                           TEXT("당신 덕분에 조금 용기가 났어요.") }, Name, 2);
    case ENPCPersonality::Intellectual:
        return PickLines({ TEXT("자네와의 대화는 늘 배울 게 있어. 귀한 인연이야."), TEXT("자네라면 내 얘길 이해해 주겠지."),
                           TEXT("자네 같은 사람을 만난 건 행운이야.") }, Name, 2);
    case ENPCPersonality::Tough:
        return PickLines({ TEXT("흥... 너 정도면 등을 맡길 만해."), TEXT("인정한다. 넌 진짜배기야."),
                           TEXT("너랑이라면 한판 더 뛸 수 있겠어.") }, Name, 2);
    case ENPCPersonality::Kind:
        return PickLines({ TEXT("당신을 만난 게 참 다행이에요."), TEXT("힘들 땐 꼭 나한테 기대요, 알았죠?"),
                           TEXT("당신은 내게 소중한 사람이에요.") }, Name, 2);
    case ENPCPersonality::Cool:
        return PickLines({ TEXT("……너한테는, 조금은 솔직해져도 되겠군."), TEXT("이런 말 잘 안 하는데. ……고맙다."),
                           TEXT("……곁에 있어도 나쁘진 않아.") }, Name, 2);
    default:
        return {};
    }
}

// ── 스토리 반응 대사 (성격 × 사건 단계) ───────────────────
// 모든 NPC가 메인 스토리 진행에 반응 → 거리 전체가 살아있는 무대가 된다(데이터 입력 0).
// 4단계: 공허현상 불안(Prologue) → 흑마술사 공포(WarlockRumor) → 격파 후 안도+여운(WarlockDefeated) → 평화 회복(StoryClear).
static TArray<FStoryReactiveLines> BuildStoryReactiveLines(ENPCPersonality P, const FString& Name)
{
    // 공개적으로 체감되는 5단계: 불안→공포소문→결전임박(거리 텅 빔)→안도→회복.
    // (각성/팀결성/최종보스는 비공개=가면 뒤 싸움이라 일반 시민 대사엔 미반영.)
    auto Make = [&](const TArray<FString>& Pro, const TArray<FString>& Rumor,
                    const TArray<FString>& Located, const TArray<FString>& Won,
                    const TArray<FString>& Peace)
    {
        TArray<FStoryReactiveLines> Out;
        Out.Add({ TEXT("Prologue"),        PickLines(Pro,     Name, 2) });
        Out.Add({ TEXT("WarlockRumor"),    PickLines(Rumor,   Name, 2) });
        Out.Add({ TEXT("WarlockLocated"),  PickLines(Located, Name, 2) });
        Out.Add({ TEXT("WarlockDefeated"), PickLines(Won,     Name, 2) });
        Out.Add({ TEXT("StoryClear"),      PickLines(Peace,   Name, 2) });
        return Out;
    };

    switch (P)
    {
    case ENPCPersonality::Cheerful:
        return Make(
            { TEXT("야, 들었어? 또 누가 길에서 픽 쓰러졌대. 요즘 무섭다 진짜…"),
              TEXT("나 멀쩡하지? 갑자기 막 무기력해진다는 거, 그거 좀 소름이야."),
              TEXT("기운 내자! 이럴 때일수록 웃어야지, 안 그래?") },
            { TEXT("'흑마술사'? 그런 게 진짜 있대. 밤에 돌아다니지 말랬어, 우리 엄마가."),
              TEXT("요즘 거리 분위기 왜 이래… 다들 눈치만 보고. 빨리 좀 끝났으면."),
              TEXT("나 솔직히 무서워. 근데 너 보면 좀 괜찮아진다? 신기하지?") },
            { TEXT("야, 다들 집에 박혀서 거리가 텅 비었어. 가게도 절반은 문 닫았더라."),
              TEXT("뭔가… 곧 큰일이 터질 것 같지 않아? 공기가 막 무거워. 너도 조심해!") },
            { TEXT("그 흑마술사 잡혔다며?! 와, 대박! 너희가 한 거 아냐? 어?"),
              TEXT("거리가 좀 밝아진 느낌이야. 근데 아직 뭔가 찜찜한 건 나뿐인가?"),
              TEXT("쓰러지는 사람 좀 줄었대! 이제 진짜 끝이면 좋겠다.") },
            { TEXT("야~ 날씨 좋다! 이런 평범한 아침이 이렇게 좋은 거였나 봐."),
              TEXT("다 끝났대! 우리 이거 기념으로 노래방 가야 하는 거 아냐?!"),
              TEXT("요즘 다들 표정이 살아났어. 역시 세상은 이래야지!") });
    case ENPCPersonality::Shy:
        return Make(
            { TEXT("저… 사람들이 자꾸 쓰러져요. 저도 그렇게 될까 봐… 무서워요."),
              TEXT("밖에 나오는 게 좀… 겁나요. 그래도, 집에만 있긴 답답해서.") },
            { TEXT("가면 쓴 사람 봤다는 얘기… 저는 그냥, 안 듣고 싶어요."),
              TEXT("밤이 길게 느껴져요. 빨리… 누가 이걸 끝내줬으면.") },
            { TEXT("다들 밖에 안 나와요. 거리가 무서울 만큼 조용해요. …뭔가, 곧 일어날 것 같아요."),
              TEXT("저… 무사히 돌아오실 거죠? 꼭, 꼭이요.") },
            { TEXT("그게… 잡혔다고 들었어요. 조금은, 발 뻗고 잘 수 있을까요."),
              TEXT("아직 무섭긴 한데… 그래도 전보다는, 나아요. 정말로.") },
            { TEXT("이제… 밤에도 무섭지 않아요. 이런 날이 올 줄 몰랐어요."),
              TEXT("저, 요즘은 밖에 나오는 게 조금 좋아졌어요. …덕분에요.") });
    case ENPCPersonality::Intellectual:
        return Make(
            { TEXT("이 '공허 현상', 의학적으론 설명이 안 돼. 원인이 따로 있다는 거지."),
              TEXT("통계가 이상해. 발병이 특정 시간대·장소에 몰려 있어. 우연이 아니야.") },
            { TEXT("'흑마술사'라… 비과학적이군. 하지만 데이터는 그쪽을 가리켜."),
              TEXT("뜬소문이라기엔 패턴이 너무 또렷해. 누군가 의도적으로 벌이는 일이야.") },
            { TEXT("발병 곡선이 정점에 가까워. 임계점이지. 곧 한 번에 터지거나, 한 번에 멎거나 — 둘 중 하나야."),
              TEXT("거리가 비었군. 사람들의 공포가 임계에 달했다는 신호야. 흥미로운 동시에, 위험하지.") },
            { TEXT("배후가 정리됐다고? 흥미롭군. 한데, 진짜 원인이 그자였을까?"),
              TEXT("표면적 원흉은 사라졌어. 그래도 난… 더 큰 변수가 남았다고 봐.") },
            { TEXT("발병 그래프가 완전히 꺾였어. 통계가 거짓말을 안 하지. 끝난 거야."),
              TEXT("자네들이 한 일, 언젠가 제대로 기록될 거야. 그럴 가치가 있어.") });
    case ENPCPersonality::Tough:
        return Make(
            { TEXT("쯧, 멀쩡하던 놈이 픽픽 쓰러져. 이게 뭔 꼴이야."),
              TEXT("겁먹을 거 없어. 와도 정면으로 받으면 돼. …근데 상대가 안 보인다는 게 문제지.") },
            { TEXT("가면 쓴 자식이 뒤에서 수작을 부린다고? 잡히기만 해봐라."),
              TEXT("거리가 흉흉해. 약한 놈부터 당하는 거야. 정신 똑바로 차려.") },
            { TEXT("다들 꽁무니를 뺐군. 거리가 텅 비었어. …이럴 때일수록 누군가는 서 있어야지."),
              TEXT("결판 낼 때가 온 거 같다. 갈 거면, 제대로 끝장을 봐라.") },
            { TEXT("그 자식 박살냈다며. 잘했다. 그래야 본때를 보이지."),
              TEXT("아직 끝난 거 같진 않아. 긴장 풀지 마. 진짜는 지금부터다.") },
            { TEXT("끝났군. …흥, 제법이야. 너희, 인정한다."),
              TEXT("이제 두 다리 뻗고 자겠어. 고생했다, 진심으로.") });
    case ENPCPersonality::Kind:
        return Make(
            { TEXT("어머, 또 누가 쓰러졌다고… 다들 무리하지 말고 푹 쉬어요, 응?"),
              TEXT("요즘 다들 지쳐 보여요. 따뜻한 거라도 챙겨 먹어요, 꼭.") },
            { TEXT("무서운 소문이 돌아도… 너무 겁먹지 말아요. 같이 있으면 괜찮아요."),
              TEXT("밤늦게 다니지 말아요. 무슨 일 생기면, 나한테 꼭 연락하고.") },
            { TEXT("다들 문을 걸어 잠갔어요. 거리가 이렇게 조용한 건 처음이에요. …부디 몸조심해요."),
              TEXT("어디 가는진 안 물을게요. 그저… 꼭 무사히 돌아와요. 약속해 줘요.") },
            { TEXT("그 무서운 게 잡혔다니 다행이에요. 다친 데는 없죠? 정말?"),
              TEXT("아직 마음 놓긴 이르지만… 그래도, 한결 안심돼요.") },
            { TEXT("이제 다들 편히 쉴 수 있겠어요. 정말, 고마운 일이에요."),
              TEXT("당신 덕분이에요. 이런 평화로운 날을 다시 보다니… 고마워요.") });
    case ENPCPersonality::Cool:
        return Make(
            { TEXT("……사람들이 쓰러진다. 원인 불명. 좋지 않은 흐름이군."),
              TEXT("소란스럽군. 하지만 호들갑 떤다고 해결될 일은 아니지.") },
            { TEXT("흑마술사라. ……소문의 출처를 쫓으면, 뭔가 나오겠지."),
              TEXT("거리가 가라앉았어. ……누군가는 움직여야 할 때야.") },
            { TEXT("……거리가 비었군. 폭풍 전야란 이런 거지."),
              TEXT("때가 됐어. ……돌아올 자신 없으면, 안 가는 게 나아. 갈 거면, 끝을 봐.") },
            { TEXT("끄나풀 하나 잡았다고 끝은 아니야. ……방심하지 마라."),
              TEXT("표면은 정리됐군. 진짜 배후는, 아직.") },
            { TEXT("……조용해졌군. 나쁘지 않아."),
              TEXT("끝났나. ……수고했다. 이런 말, 자주 안 하니 새겨들어.") });
    default:
        return {};
    }
}

// ── 일상 균열 대사 (캐서린풍 — 단서 수집 후 사건 해결 전) ─
// 친숙한 NPC가 잠깐 '저쪽'에 물들어 이상한 말을 흘리고는 곧장 잊는다. 불쾌한 골짜기로 진실에 다가가는 긴장.
static TArray<FString> BuildCreepyLines(ENPCPersonality P, const FString& Name)
{
    switch (P)
    {
    case ENPCPersonality::Cheerful:
        return PickLines({ TEXT("어! 왔어~ ……너도 곧, 편해질 거야. ……응? 내가 방금 뭐랬지? 하하, 잠을 못 자서 그런가."),
                           TEXT("놀자! 다 내려놓고… 그냥… 가라앉으면… ——어, 미안. 갑자기 졸려서. 무슨 얘기 하고 있었지?") }, Name, 1);
    case ENPCPersonality::Shy:
        return PickLines({ TEXT("저… 안 힘들어요? 다… 그만둬도 돼요. 편하게… ——어? 제, 제가 무슨 말을…? 죄송해요, 갑자기."),
                           TEXT("거울 보지 마세요. 거기 있는 건… 아, 아니에요. 그냥… 조심히 가요.") }, Name, 1);
    case ENPCPersonality::Intellectual:
        return PickLines({ TEXT("저항은 비효율적이야. 모두가 내려놓으면, 평형에 도달하지. 그게 가장… ——음? 내가 무슨 소릴 한 거지. 이상하군."),
                           TEXT("데이터가 말해. 곧 모두가 하나로… ——아니. 방금 건 내 생각이 아니야. 누가 대신 말한 것 같았어.") }, Name, 1);
    case ENPCPersonality::Tough:
        return PickLines({ TEXT("버텨봐야 소용없어. 약한 건… 다스려지는 게 편해. ——뭐? 내가 그딴 소릴? 제기랄, 머리가 안 돌아가는군."),
                           TEXT("싸우지 마. 그냥… 무릎 꿇으면… ——크윽, 아니다. 헛소리 집어치워. 잊어.") }, Name, 1);
    case ENPCPersonality::Kind:
        return PickLines({ TEXT("어서 와요~ 다 괜찮아질 거예요. 아무것도 안 해도 되니까… 그냥 놓아버려요. ——어머, 내가 왜 이런 말을? 미안해요."),
                           TEXT("푹 쉬어요. 영원히… 깨지 않아도… ——어? 무슨 말을 하려던 거였죠? 정신이 깜빡했네.") }, Name, 1);
    case ENPCPersonality::Cool:
        return PickLines({ TEXT("……저항하는 건 너뿐이야. 다들 이미 내려놨는데. ……뭐? 방금 그건 내 목소리가 아니었어."),
                           TEXT("……곧 너도 우리처럼. ——아니. 지금 건 잊어. 나도, 왜 이런 말을 했는지 모르겠으니까.") }, Name, 1);
    default:
        return {};
    }
}

// ── 양산 카탈로그 ─────────────────────────────────────────
// 항목 1개 = NPC 1명. 사용자는 BP의 ArchetypeId만 맞추고 메시/애니만 입히면 됨.
// 대사 배열을 비운 항목은 성격 기반 자동 생성됨(ApplySocialProfile에서 처리).
const TArray<FNPCSocialProfile>& UNPCArchetypeLibrary::GetCatalog()
{
    static TArray<FNPCSocialProfile> Catalog;
    if (Catalog.Num() > 0) return Catalog;

    auto Add = [](FName Id, const FString& Name, ENPCPersonality Pers)
        -> FNPCSocialProfile&
    {
        FNPCSocialProfile P;
        P.ArchetypeId = Id;
        P.DisplayName = Name;
        P.Personality = Pers;
        const int32 Index = Catalog.Add(P);
        return Catalog[Index];
    };

    // ── 현대 배경(페르소나풍) 캐스트 — 현대 한국 도시물 ──
    // 1) 활발한 고등학생 — 낮, 영입(인연3)
    {
        FNPCSocialProfile& P = Add(TEXT("char_01"), TEXT("지훈"), ENPCPersonality::Cheerful);
        P.ActivePhases = { EDayPhase::Morning, EDayPhase::Day };
        P.FavoriteGiftId = TEXT("HealPotion");
        P.bCanBeRecruited = true; P.RecruitMinBondRank = 3;
        P.RecruitLine = TEXT("좋아, 나도 낄게! 같이 뛰면 무서울 거 없지. 끝까지 가는 거다!");
        P.DialogueLines = {
            TEXT("어, 왔어?! 야 오늘따라 컨디션 좋아 보인다? 같이 좀 뛸래?"),
            TEXT("나 사실 가만히 있는 거 진짜 못 해. 몸을 움직여야 머리가 맑아지더라고!"),
            TEXT("뭔 일 있어도 일단 웃자. 웃으면 어떻게든 되더라, 진짜로.") };
        P.BondDialogueLines = {
            TEXT("야, 솔직히 말할게. 나 항상 밝은 거… 분위기 처지는 게 무서워서 그래."),
            TEXT("근데 너한텐 안 꾸며도 되더라. 그게 얼마나 편한지 몰라. 고맙다, 진짜.") };
    }
    // 2) 내성적인 도서부원 — 선물 좋아함
    {
        FNPCSocialProfile& P = Add(TEXT("char_02"), TEXT("서연"), ENPCPersonality::Shy);
        P.FavoriteGiftId = TEXT("HiPotion"); P.GiftAffinity = 10;
        P.DialogueLines = {
            TEXT("아… 안녕하세요. 저, 지금 막 좋은 구절을 읽던 참이라…"),
            TEXT("사람 많은 데보다, 이런 조용한 데가 좋아요. 책이랑 있으면 안 무섭거든요."),
            TEXT("그, 혹시… 방해됐다면 죄송해요. 저는 여기 늘 있으니까, 또 와도 돼요.") };
        P.EveningDialogueLines = {
            TEXT("해 질 녘 도서관은… 제일 좋아하는 시간이에요. 아무도 없어서."),
            TEXT("밤엔 좀 무섭지만… 이상하게, 글은 밤에 더 잘 써져요.") };
        P.BondDialogueLines = {
            TEXT("저… 사실 소설을 써요. 아무한테도 안 보여줬는데, 당신한테는 말하고 싶었어요."),
            TEXT("당신과 있으면, 현실에서도 한 페이지쯤 더 넘겨볼 용기가 나요. 정말로.") };
    }
    // 3) 지적인 담임 선생님 — 퀘스트 제공, 낮/저녁(야자)
    {
        FNPCSocialProfile& P = Add(TEXT("char_03"), TEXT("윤재 선생님"), ENPCPersonality::Intellectual);
        P.GrantsQuestId = TEXT("quest_lore");
        P.ActivePhases = { EDayPhase::Day, EDayPhase::Evening };
        P.DialogueLines = {
            TEXT("어, 우리 반. 표정이 좀 달라졌구나. …뭔가 짊어진 사람의 얼굴이야."),
            TEXT("선생이 다 알 순 없지만, 하나는 안다. 혼자 끙끙대지 마라. 그게 제일 위험해."),
            TEXT("요즘 학교가 어수선하지? …나도 느껴. 그래서 너희를 더 눈여겨보게 되더라.") };
        P.BondDialogueLines = {
            TEXT("나는 학생을 '가르치는' 사람이라고 생각했는데, 요즘은 너한테 배우는 게 더 많다."),
            TEXT("무슨 일이 있어도 학교는, 내가 지킬게. 너희가 돌아올 평범한 자리는 있어야 하니까.") };
    }
    // 4) 거친 복싱부 주장 — 영입(인연5)
    {
        FNPCSocialProfile& P = Add(TEXT("char_04"), TEXT("강현"), ENPCPersonality::Tough);
        P.bCanBeRecruited = true; P.RecruitMinBondRank = 5;
        P.FavoriteGiftId = TEXT("HiPotion");
        P.RecruitLine = TEXT("…너랑이라면 등을 맡길 만하지. 좋아, 같이 간다. 내 주먹, 네 편이다.");
        P.DialogueLines = {
            TEXT("뭐야, 또 너냐. …흥, 마침 잘 왔다. 미트 좀 잡아줄래?"),
            TEXT("난 약한 게 싫어. 약하면, 지키고 싶은 걸 못 지키니까."),
            TEXT("쓸데없는 소린 안 해. 덤빌 거면 제대로, 갈 거면 똑바로 가.") };
        P.BondDialogueLines = {
            TEXT("예전엔 센 주먹이 강함인 줄 알았어. 지금은 아니야. 지킬 게 있는 게 강한 거더라."),
            TEXT("무슨 일이 와도 네 등은 내가 지킨다. …이런 말 쑥스러우니까 한 번만 한다.") };
    }
    // 5) 상냥한 보건 선생님 — 영입(인연4)
    {
        FNPCSocialProfile& P = Add(TEXT("char_05"), TEXT("민지 쌤"), ENPCPersonality::Kind);
        P.bCanBeRecruited = true; P.RecruitMinBondRank = 4;
        P.FavoriteGiftId = TEXT("Antidote");
        P.RecruitLine = TEXT("내가 도울게. 다친 사람 돌보는 건 내 일이니까. …이번엔, 너희 곁에서.");
        P.DialogueLines = {
            TEXT("어서 와요, 보건실은 언제든 열려 있어요. 어디 아픈 덴 없죠?"),
            TEXT("다친 데 없어도 괜찮으니까, 힘들면 그냥 와서 좀 쉬다 가요."),
            TEXT("요즘 쓰러지는 애들이 많아서… 다들 너무 무리하는 것 같아 걱정이에요.") };
        P.BondDialogueLines = {
            TEXT("늘 남을 돌보는 입장이었는데… 네가 매번 내 안부를 물어주잖아. 그게 참 고마워."),
            TEXT("이제 나도 누군가한테 기댈 수 있을 것 같아. 네 덕분에, 다시 돌볼 힘이 났어.") };
    }
    // 6) 냉정한 사립탐정 — 밤에만 출현, 영입(인연6)
    {
        FNPCSocialProfile& P = Add(TEXT("char_06"), TEXT("탐정 진"), ENPCPersonality::Cool);
        P.ActivePhases = { EDayPhase::Night };
        P.bCanBeRecruited = true; P.RecruitMinBondRank = 6;
        P.RecruitLine = TEXT("……혼자 일하는 주의지만, 너는 예외로 하지. 이 사건, 끝까지 같이 파보자고. 파트너.");
        P.DialogueLines = {
            TEXT("……또 너군. 이 시간에 돌아다니는 걸 보면, 너도 평범한 부류는 아니야."),
            TEXT("나는 사실만 믿어. 심증은 증거가 아니지. ……하지만 이번 건은, 심증이 너무 강해."),
            TEXT("질문은 짧게. 밤은 길어도, 단서는 금방 사라지거든.") };
        P.EveningDialogueLines = {
            TEXT("밤은 거짓말이 벗겨지는 시간이야. 낮의 가면을 다들 벗거든. ……그래서 단서도 밤에 나오지."),
            TEXT("이 시간에 깨어 있는 자는 둘뿐이야. 쫓는 자와, 쫓기는 자. ……너는 어느 쪽이지?") };
        P.BondDialogueLines = {
            TEXT("난 누굴 믿었다 크게 다친 적이 있어. 그 뒤로 혼자 일했지. ……네 앞에선 그 원칙, 접어둘게."),
            TEXT("내 인생 최고의 발견은 사건이 아니라 너였어. 어떤 미궁이 와도, 함께면 풀 수 있어. 파트너.") };
    }
    // 7) 활발한 편의점 점장 — 낮 상점, 밤 영업종료
    {
        FNPCSocialProfile& P = Add(TEXT("char_07"), TEXT("점장 태수"), ENPCPersonality::Cheerful);
        P.bIsShopkeeper = true; P.bClosedAtNight = true; P.ShopKind = TEXT("convenience");
        P.ActivePhases = { EDayPhase::Morning, EDayPhase::Day, EDayPhase::Evening };
        P.DialogueLines = {
            TEXT("어서 와~! 오늘 삼각김밥 신상 들어왔어. 학생 할인도 해줄게, 단골이니까!"),
            TEXT("요즘 야간에 쓰러지는 손님이 부쩍 늘어서… 밤엔 일찍 들어가, 알았지?"),
            TEXT("편의점은 24시간이 기본인데, 요샌 무서워서 밤 장사를 접었다니까. 세상 참.") };
    }
    // 8) 냉정한 심야 바 바텐더 — 밤 상점
    {
        FNPCSocialProfile& P = Add(TEXT("char_08"), TEXT("바텐더 레이"), ENPCPersonality::Cool);
        P.bIsShopkeeper = true; P.ShopKind = TEXT("bar");
        P.ActivePhases = { EDayPhase::Night };
        P.DialogueLines = {
            TEXT("…어서 와. 'Stray'에 온 걸 환영해. 고민은 문 앞에 두고 들어와."),
            TEXT("무알콜도 있으니까 학생도 괜찮아. 잠 안 오는 밤엔, 여기 흘러드는 사람이 많거든."),
            TEXT("외로운 사람끼리 한 공간에 있는 것만으로도, 조금은 덜 외로워지더라고. …그게 이 가게의 메뉴야.") };
        P.EveningDialogueLines = {
            TEXT("이 시간에 안 자고 뭐 해. …아니, 됐어. 여기 오는 사람한테 '왜'는 안 물어."),
            TEXT("요즘 밤손님이 늘었어. 다들 집에 가도 잠이 안 온대. …그 마음, 나도 알지.") };
    }
    // 9) 상냥한 분식집 아주머니 — 선물/심부름 퀘스트
    {
        FNPCSocialProfile& P = Add(TEXT("char_09"), TEXT("분식집 순자씨"), ENPCPersonality::Kind);
        P.FavoriteGiftId = TEXT("HealPotion");
        P.GrantsQuestId = TEXT("quest_errand");
        P.DialogueLines = {
            TEXT("아이고 우리 학생 왔능가! 떡볶이 막 만들어놨어, 좀 앉아서 먹고 가."),
            TEXT("많이 먹어, 응? 다 먹고살자고 하는 일이여. 굶지 말고."),
            TEXT("요새 다들 기운이 없어 보여. 그럴수록 따뜻한 거 한 그릇이 약이여, 약.") };
        P.BondDialogueLines = {
            TEXT("자네는 꼭 손주 같어. 힘든 일 있으면 여기 와서 풀고 가, 알았제?"),
            TEXT("이 분식집이 뭐 대단한 건 아녀도, 사람들 마음 쉬어가는 자리는 되드라고. 그거면 됐지.") };
    }
    // 10) 지적인 공대생 해커 — 영입(인연3)
    {
        FNPCSocialProfile& P = Add(TEXT("char_10"), TEXT("은우"), ENPCPersonality::Intellectual);
        P.bCanBeRecruited = true; P.RecruitMinBondRank = 3;
        P.FavoriteGiftId = TEXT("SPPotion");
        P.RecruitLine = TEXT("접속 허가. …농담이고. 너랑이면 화면 밖도 나쁘지 않겠더라. 내 키보드, 너희 편이야.");
        P.DialogueLines = {
            TEXT("……어. 말 걸 거면 용건만. 나 지금 패킷 분석 중이라."),
            TEXT("사람은 거짓말을 하는데 데이터는 안 해. 그래서 난 화면 뒤가 편하더라."),
            TEXT("그 '공허 현상' 말야, 발병 좌표를 찍어봤거든? …패턴이 있어. 누가 의도한 거라고.") };
        P.BondDialogueLines = {
            TEXT("온라인에선 다들 날 떠받드는데, 정작 내 이름 '은우'는 아무도 몰라. …너는 알잖아."),
            TEXT("이건 영구 접속이야. 로그아웃 안 해. 절대로. 평생 핑 보낼 거니까 각오해.") };
    }
    // 11) 거친 오토바이 정비공 — 낮 상점
    {
        FNPCSocialProfile& P = Add(TEXT("char_11"), TEXT("정비공 두식"), ENPCPersonality::Tough);
        P.bIsShopkeeper = true; P.ShopKind = TEXT("gear");
        P.ActivePhases = { EDayPhase::Morning, EDayPhase::Day };
        P.DialogueLines = {
            TEXT("어, 왔냐. 손은 기름투성이라 악수는 됐고. 뭐 고칠 거라도 있어?"),
            TEXT("엔진은 혼자 안 돌아. 부품 하나만 빠져도 전체가 멈추지. …사람도 똑같더라."),
            TEXT("고장 난 건 버리는 게 아니라 고치는 거다. 시간이 좀 걸려도 말이야.") };
    }
    // 12) 내성적인 버스킹 뮤지션 — 저녁/밤 출현
    {
        FNPCSocialProfile& P = Add(TEXT("char_12"), TEXT("버스커 하늘"), ENPCPersonality::Shy);
        P.ActivePhases = { EDayPhase::Evening, EDayPhase::Night };
        P.FavoriteGiftId = TEXT("HiPotion");
        P.GrantsQuestId = TEXT("sq_lost_song");
        P.DialogueLines = {
            TEXT("아… 들으셨어요? 제 노래. 사람 앞에선 떨려서, 거리에서나 겨우 불러요."),
            TEXT("가사는… 하고 싶은 말을 못 할 때 쓰는 거예요. 노래로는, 조금 용감해지거든요."),
            TEXT("이 시간 거리는 조용해서 좋아요. 제 목소리가 누군가한테 가닿으면… 그걸로 충분해요.") };
        P.EveningDialogueLines = {
            TEXT("밤 버스킹은… 관객이 적어서 오히려 편해요. 한두 명이라도 멈춰 서주면, 그날은 성공이에요."),
            TEXT("요즘은 노래로 사람들을 좀 위로하고 싶어요. 다들 지쳐 보여서… 그게 제가 할 수 있는 전부라.") };
        P.BondDialogueLines = {
            TEXT("다음 곡은… 당신을 생각하면서 썼어요. 부끄러우니까, 제목은 비밀이에요."),
            TEXT("당신이 들어주니까, 이제 사람들 앞에서도 부를 수 있을 것 같아요. 정말로요.") };
    }
    // 13) 활발한 운동부 매니저 — 낮, 영입(인연3)
    {
        FNPCSocialProfile& P = Add(TEXT("char_13"), TEXT("보라"), ENPCPersonality::Cheerful);
        P.ActivePhases = { EDayPhase::Morning, EDayPhase::Day };
        P.FavoriteGiftId = TEXT("HealPotion");
        P.GrantsQuestId = TEXT("Q_FindTreasure"); // 탐험 사이드 퀘스트 체인 진입점(메인 스토리와 별개)
        P.bCanBeRecruited = true; P.RecruitMinBondRank = 3;
        P.RecruitLine = TEXT("뒤는 내가 받칠게! 앞에서 마음 놓고 싸워. 그게 매니저의 일이니까!");
        P.DialogueLines = {
            TEXT("어, 왔구나! 물 마실래? 수건도 있어. 내가 또 이런 건 잘 챙기거든."),
            TEXT("매니저는 늘 뒤에서 챙기는 자리야. 근데 그게 싫진 않아. 누군가는 해야 하니까!"),
            TEXT("선수들이 마음 놓고 뛰려면 뒤가 든든해야지. 그게 내 몫이고.") };
        P.BondDialogueLines = {
            TEXT("늘 남 챙기다 보니, 내가 뭘 좋아했는지도 까먹었더라. 네가 처음 물어봐 줬어."),
            TEXT("네가 앞에서 싸울 때, 내가 뒤를 받칠게. 그게 내 역할이고, 내가 제일 잘하는 거니까!") };
    }
    // 14) 냉정한 학생회장 — 낮/저녁, 퀘스트 제공, 영입(인연5)
    {
        FNPCSocialProfile& P = Add(TEXT("char_14"), TEXT("학생회장 도현"), ENPCPersonality::Cool);
        P.ActivePhases = { EDayPhase::Day, EDayPhase::Evening };
        P.GrantsQuestId = TEXT("Q_BountyHunt");
        P.bCanBeRecruited = true; P.RecruitMinBondRank = 5;
        P.RecruitLine = TEXT("회장으로서가 아니라, 그냥 도현으로서 함께하지. …이런 결정도 가끔은, 나쁘지 않네.");
        P.DialogueLines = {
            TEXT("학생회장 도현이야. 무슨 일이지? 건의사항이면 정식 절차를 밟아줘."),
            TEXT("학교의 질서는 내가 책임진다. ……그게 가끔 무겁긴 하지만, 누군가는 해야 하니까."),
            TEXT("완벽해 보인다고? 그렇게 보이려고 애쓰는 것뿐이야. ……아니, 방금 건 못 들은 걸로 해.") };
        P.BondDialogueLines = {
            TEXT("모두가 기대하는 '완벽한 회장'… 그게 가끔 숨이 막혀. 네 앞에선 실수해도 될 것 같아."),
            TEXT("회장도 에이스도 아닌, 그냥 '도현'으로 있을 수 있는 곳이 생겼어. 그게 너야. 잊지 않을게.") };
    }
    // 15) 상냥한 카페 바리스타 — 낮 카페(상점)
    {
        FNPCSocialProfile& P = Add(TEXT("char_15"), TEXT("바리스타 유나"), ENPCPersonality::Kind);
        P.bIsShopkeeper = true; P.bClosedAtNight = true; P.ShopKind = TEXT("cafe");
        P.ActivePhases = { EDayPhase::Morning, EDayPhase::Day, EDayPhase::Evening };
        P.FavoriteGiftId = TEXT("SPPotion");
        P.DialogueLines = {
            TEXT("어서 오세요~ 오늘의 원두는 좀 산뜻해요. 따뜻한 거로 한 잔 내려드릴까요?"),
            TEXT("커피 한 잔의 여유라는 말, 요즘은 진짜 필요한 것 같아요. 다들 너무 바쁘게 사니까."),
            TEXT("힘들 땐 잠깐 앉았다 가요. 창가 자리, 비워둘게요.") };
    }
    // 16) 지적인 타로 점술가 — 밤 출현
    {
        FNPCSocialProfile& P = Add(TEXT("char_16"), TEXT("점술가 셀린"), ENPCPersonality::Intellectual);
        P.ActivePhases = { EDayPhase::Night };
        P.FavoriteGiftId = TEXT("Antidote");
        P.GrantsQuestId = TEXT("sq_tarot_reading");
        P.DialogueLines = {
            TEXT("……기다리고 있었어. 카드가 네 얘길 하더라고. 앉아, 한 장 뽑아줄게."),
            TEXT("미래는 정해진 게 아니야. 다만 '흐름'이 보일 뿐. 그 흐름을 바꾸는 건 네 선택이고."),
            TEXT("요즘 카드가 자꾸 '탑'을 내놔. 무언가 크게 무너진다는 뜻이지. …이미 시작됐을지도.") };
        P.EveningDialogueLines = {
            TEXT("밤은 '저쪽'이 가까워지는 시간이야. 거울을 너무 오래 보지 마. …진심으로 하는 말이야."),
            TEXT("달이 흐린 밤엔 가면 쓴 손님이 거리에 선대. 마주치면, 눈을 피하렴.") };
    }
    // 17) 거친 배달 라이더 — 저녁/밤, 영입(인연4)
    {
        FNPCSocialProfile& P = Add(TEXT("char_17"), TEXT("라이더 철민"), ENPCPersonality::Tough);
        P.ActivePhases = { EDayPhase::Evening, EDayPhase::Night };
        P.bCanBeRecruited = true; P.RecruitMinBondRank = 4;
        P.FavoriteGiftId = TEXT("HiPotion");
        P.RecruitLine = TEXT("좋아, 시동 건다. 너희가 내 돌아올 곳이니까 — 어디든 같이 달리자.");
        P.DialogueLines = {
            TEXT("배달 가는 길이었는데, 뭐 잠깐은 괜찮아. 헬멧은 벗고 얘기하자."),
            TEXT("바이크로 달리면 다 잊혀져. 바람 소리밖에 안 들리거든. …근데 도망치는 거랑은 달라."),
            TEXT("이 도시 골목은 내가 제일 잘 알아. 어디가 위험한지도. 요즘은… 밤골목이 좀 이상해.") };
        P.EveningDialogueLines = {
            TEXT("밤 배달이 제일 바빠. 근데 요샌 골목에서 이상한 한기가 느껴져. …기분 탓이겠지."),
            TEXT("밤바람 가르고 달리면 머리가 비워져. 태워줄까? 농담이야. …아니, 진심 반.") };
        P.BondDialogueLines = {
            TEXT("계속 달리기만 했어. 멈추면 뭔가 따라잡힐 것 같아서. 근데 너랑 있으면 시동 꺼도 괜찮더라."),
            TEXT("돌아올 곳이 있으니까 더 멀리 갈 수 있는 거더라. 너희가 내 '돌아올 곳'이야.") };
    }
    // 18) 내성적인 미술부원 — 낮, 선물 좋아함
    {
        FNPCSocialProfile& P = Add(TEXT("char_18"), TEXT("하린"), ENPCPersonality::Shy);
        P.ActivePhases = { EDayPhase::Day, EDayPhase::Evening };
        P.FavoriteGiftId = TEXT("HiPotion"); P.GiftAffinity = 10;
        P.DialogueLines = {
            TEXT("아… 보고 계셨어요? 아직 미완성이라 좀… 부끄러운데."),
            TEXT("말로 못 하는 건, 그림으로 그려요. 색이 대신 말해주거든요."),
            TEXT("요즘은 자꾸 회색만 칠하게 돼요. 거리가… 그렇게 보여서. 빨리 색이 돌아왔으면.") };
        P.BondDialogueLines = {
            TEXT("이 그림… 당신을 그린 거예요. 봐도 돼요. 아니, 봐줬으면 좋겠어요."),
            TEXT("당신이랑 있으면 그리고 싶은 게 자꾸 생겨요. 회색 말고, 따뜻한 색으로요.") };
    }
    // 19) 활발한 방송부 DJ — 저녁/밤
    {
        FNPCSocialProfile& P = Add(TEXT("char_19"), TEXT("DJ 제이"), ENPCPersonality::Cheerful);
        P.ActivePhases = { EDayPhase::Evening, EDayPhase::Night };
        P.FavoriteGiftId = TEXT("SPPotion");
        P.DialogueLines = {
            TEXT("예~ 한울의 밤을 책임지는 DJ 제이! 사연 있으면 말해, 다음 곡으로 틀어줄게!"),
            TEXT("야자 끝나고 옥상 봤는데… 가면 쓴 누가 서 있더라? 눈 깜빡이니까 없어졌어. 으, 진짜야!"),
            TEXT("음악은 말야, 혼자 듣는 거 같아도 사실 다 같이 듣는 거거든. 그게 좋아.") };
        P.EveningDialogueLines = {
            TEXT("심야방송 시작! …이라기엔 청취자가 너랑 나뿐인 것 같지만. 뭐 어때, 둘이면 라디오지!"),
            TEXT("밤에 사연이 제일 많이 와. 다들 낮엔 못 하는 얘길, 밤에 털어놓더라고.") };
    }
    // 20) 상냥한 동물병원 수의사 — 낮, 영입(인연4)
    {
        FNPCSocialProfile& P = Add(TEXT("char_20"), TEXT("수의사 선우"), ENPCPersonality::Kind);
        P.ActivePhases = { EDayPhase::Morning, EDayPhase::Day };
        P.bCanBeRecruited = true; P.RecruitMinBondRank = 4;
        P.FavoriteGiftId = TEXT("Antidote");
        P.RecruitLine = TEXT("말 못 하는 것들을 돌보다 왔어. 이번엔 너희를 돌볼게. 한 명도, 안 잃을 거야.");
        P.DialogueLines = {
            TEXT("어서 와요. 지금 막 고양이 한 마리 퇴원시켰어요. 잘 회복돼서 다행이지."),
            TEXT("동물은 아프다고 말을 못 해요. 그래서 더 잘 들여다봐야 하죠. …사람도 똑같더라고요."),
            TEXT("혼자 끙끙 앓지 말아요. 말 못 하는 것들 보다 보면, 그게 제일 안타깝거든요.") };
        P.BondDialogueLines = {
            TEXT("못 살린 아이가 하나 있어요. 아직도 그 눈빛이 떠올라요. …네가 들어줘서, 오늘은 좀 가벼워."),
            TEXT("네 곁이라면, 한 명이라도 더 지킬 수 있을 것 같아. 그러면 그 무력함도, 견딜 만해지겠죠.") };
    }
    // 21) 활발한 아이돌 연습생 — 저녁/밤, 영입(인연3)
    {
        FNPCSocialProfile& P = Add(TEXT("char_21"), TEXT("연습생 루나"), ENPCPersonality::Cheerful);
        P.ActivePhases = { EDayPhase::Evening, EDayPhase::Night };
        P.bCanBeRecruited = true; P.RecruitMinBondRank = 3;
        P.FavoriteGiftId = TEXT("SPPotion");
        P.RecruitLine = TEXT("나, 무대 말고 여기서도 빛날 수 있을까? …네가 그렇다면, 믿어볼래. 같이 가자!");
        P.DialogueLines = {
            TEXT("앗, 안녕! 나 방금 안무 연습 끝났어. 헤헤, 땀 범벅이라 좀 부끄럽다."),
            TEXT("데뷔? 곧… 곧 할 거야! 응, 분명히. 연습은 배신 안 하니까!"),
            TEXT("무대에 서는 상상하면 아직도 심장이 막 뛰어. 그 느낌이 좋아서 버티는 거 같아.") };
        P.EveningDialogueLines = {
            TEXT("밤 연습은 좀 외로워. 연습실 불 끄고 나올 때마다, 내가 맞게 가고 있나 싶고."),
            TEXT("그래도 야경 보면 다시 힘나! 저 불빛 중 하나는, 언젠가 날 응원해줄 사람이라고 믿거든.") };
        P.BondDialogueLines = {
            TEXT("사실 동기들은 다 데뷔했는데 나만 남았어. 웃고 있지만… 가끔 무대 뒤에서 울어."),
            TEXT("그래도 네가 '넌 빛난다'고 해주면, 한 번 더 해볼 수 있어. 첫 무대 제일 앞자리, 네 자리야!") };
    }
    // 22) 냉정한 형사 — 낮/저녁, 퀘스트 제공, 영입(인연5)
    {
        FNPCSocialProfile& P = Add(TEXT("char_22"), TEXT("강 형사"), ENPCPersonality::Cool);
        P.ActivePhases = { EDayPhase::Day, EDayPhase::Evening };
        P.GrantsQuestId = TEXT("Q_Treasures5"); // 증거 수집 의뢰(오펀 퀘스트 연결)
        P.bCanBeRecruited = true; P.RecruitMinBondRank = 5;
        P.RecruitLine = TEXT("법으로 못 잡는 놈이라면, 네 방식에 걸어보지. 배지의 무게, 같이 짊어지자고.");
        P.DialogueLines = {
            TEXT("강력계 강 형사다. ……민간인이 끼어들 사건이 아니야. 그래도, 네 눈은 좀 다르군."),
            TEXT("법으로 못 잡는 것들이 있어. 그게 형사로서 제일 분하지."),
            TEXT("이번 연쇄 무기력 사건… 증거가 자꾸 사라져. 조서도 다음 날이면 흐려지고. 처음 겪는 일이야.") };
        P.BondDialogueLines = {
            TEXT("규칙을 지키느라 놓친 진실이 있어. 그게 평생의 짐이지. …네 방식은 위험하지만, 가끔은 옳더군."),
            TEXT("법으로 못 잡는 놈을 네가 잡을 수 있을지도 모르겠어. 배지는 내가 책임진다. 함께 가지.") };
    }
    // 23) 지적인 도서관 사서 — 낮, 선물 선호
    {
        FNPCSocialProfile& P = Add(TEXT("char_23"), TEXT("사서 한지원"), ENPCPersonality::Intellectual);
        P.ActivePhases = { EDayPhase::Morning, EDayPhase::Day, EDayPhase::Evening };
        P.FavoriteGiftId = TEXT("HiPotion");
        P.GrantsQuestId = TEXT("sq_archive");
        P.DialogueLines = {
            TEXT("도서관에선 조용히. …농담이에요. 찾는 자료 있으면 말해요, 다 꿰고 있으니까."),
            TEXT("향토자료실에 흥미로운 필사본이 있어요. '군림하는 그림자'라는… 미신이겠지만, 묘하게 신경 쓰여서."),
            TEXT("책은 답을 주진 않아요. 다만 질문을 더 잘하게 도와주죠. 그거면 충분하지 않나요?") };
    }
    // 24) 상냥한 동네 의사 — 낮, 영입(인연4)
    {
        FNPCSocialProfile& P = Add(TEXT("char_24"), TEXT("정 선생님"), ENPCPersonality::Kind);
        P.ActivePhases = { EDayPhase::Morning, EDayPhase::Day };
        P.bCanBeRecruited = true; P.RecruitMinBondRank = 4;
        P.FavoriteGiftId = TEXT("Antidote");
        P.RecruitLine = TEXT("의사로서 못 살린 사람이 많아요. 네 곁이라면, 이번엔 다를 것 같아. 함께하죠.");
        P.DialogueLines = {
            TEXT("어서 와요. 동네 의원이라 별건 없지만, 어디 불편한 데 있으면 봐줄게요."),
            TEXT("요즘 깨어나지 못하는 환자가 늘었어요. 검사상으론 멀쩡한데… 의사로서 참 무력하죠."),
            TEXT("밥은 잘 챙겨 먹고 다녀요? 약보다 그게 먼저예요, 늘.") };
        P.BondDialogueLines = {
            TEXT("의사도 사람이라 모두를 살리진 못해요. 신이 아니니까. 그 무력함이 가끔 무서워."),
            TEXT("그래도 네 곁이라면 포기하지 않을 수 있을 것 같아. 한 명이라도 더, 끝까지.") };
    }
    // 25) 거친 프로게이머 — 밤, 영입(인연4)
    {
        FNPCSocialProfile& P = Add(TEXT("char_25"), TEXT("프로게이머 제로"), ENPCPersonality::Tough);
        P.ActivePhases = { EDayPhase::Night };
        P.bCanBeRecruited = true; P.RecruitMinBondRank = 4;
        P.FavoriteGiftId = TEXT("SPPotion");
        P.RecruitLine = TEXT("팀이라… 결과만 보던 내가 이런 말 하긴 그렇지만. 너희랑이면, 져도 안 떠날게.");
        P.DialogueLines = {
            TEXT("쉿. 지금 스크림 중이야. …아니, 끝났다. 졌어. 또."),
            TEXT("이 바닥은 결과가 전부야. 지면 아무도 안 봐줘. 그래서 이기는 것만 생각했지."),
            TEXT("반응속도? 자신 있어. 근데 그게 인생까지 빠르게 만들어주진 않더라.") };
        P.EveningDialogueLines = {
            TEXT("새벽까지 스크림 도는 게 일상이야. …밤이 제일 집중 잘 돼. 아무도 안 건드리니까."),
            TEXT("화면 불빛만 보고 살았어. 가끔 창밖 한 번 보는 것도… 나쁘진 않더라.") };
        P.BondDialogueLines = {
            TEXT("팀게임 하면서도 난 늘 혼자였어. 다들 결과만 봤거든. …근데 너넨, 내가 져도 안 떠나더라."),
            TEXT("우승보다 값진 걸 얻었어. 지든 이기든 안 떠날 팀. 다음 시즌도, 그 다음도, 너랑 한 팀이다.") };
    }
    // 26) 내성적인 만화방 주인 — 저녁/밤, 상점
    {
        FNPCSocialProfile& P = Add(TEXT("char_26"), TEXT("만화방 구씨"), ENPCPersonality::Shy);
        P.bIsShopkeeper = true; P.ShopKind = TEXT("manhwa");
        P.ActivePhases = { EDayPhase::Evening, EDayPhase::Night };
        P.DialogueLines = {
            TEXT("…어, 왔어요? 신간 들어왔는데. 라면도 끓여 줄까… 아니다, 귀찮으면 말고."),
            TEXT("여긴 시간이 천천히 가요. 만화 보다 보면, 바깥 일은 잠깐 잊혀지거든요."),
            TEXT("밤손님이 늘었어요. 다들 집에 가기 싫은가 봐. …뭐, 여기 있어도 돼요. 조용히만.") };
        P.EveningDialogueLines = {
            TEXT("밤엔 만화방이 제일 북적여요. 갈 곳 없는 사람들이… 여기로 흘러들거든요."),
            TEXT("불 끄기 전까진 천천히 봐도 돼요. 어차피 저도 밤엔 잠이 안 와서.") };
    }
    // 27) 활발한 꽃집 주인 — 낮, 상점
    {
        FNPCSocialProfile& P = Add(TEXT("char_27"), TEXT("꽃집 민들레"), ENPCPersonality::Cheerful);
        P.bIsShopkeeper = true; P.bClosedAtNight = true; P.ShopKind = TEXT("flower");
        P.ActivePhases = { EDayPhase::Morning, EDayPhase::Day };
        P.FavoriteGiftId = TEXT("HealPotion");
        P.DialogueLines = {
            TEXT("어서 와요! 오늘의 꽃은 물망초예요. 꽃말은 '나를 잊지 말아요'. 예쁘죠?"),
            TEXT("요즘 이 꽃 찾는 분이 많아요. 다들 누군가를, 아니면 자기 자신을, 잊고 싶지 않은가 봐."),
            TEXT("꽃 한 송이가 뭐 별거냐 싶어도, 받으면 하루가 환해지잖아요. 그게 꽃의 힘이지!") };
    }
    // 28) 냉정한 신문기자 — 낮/저녁, 퀘스트 제공
    {
        FNPCSocialProfile& P = Add(TEXT("char_28"), TEXT("윤 기자"), ENPCPersonality::Cool);
        P.ActivePhases = { EDayPhase::Day, EDayPhase::Evening };
        P.FavoriteGiftId = TEXT("SPPotion");
        P.DialogueLines = {
            TEXT("기자 윤이라고 해. 단도직입적으로 묻지. 너, 그 '일그러진 거리'를 본 적 있지?"),
            TEXT("쓰러진 사람이 일주일 새 세 배야. 병원은 원인을 못 찾고. 이게 그냥 병이 아니란 건, 너도 알잖아."),
            TEXT("진실은 늘 위험해. 그래도 누군가는 써야지. 안 그러면 다들 모른 척 잊어버리니까.") };
        P.BondDialogueLines = {
            TEXT("나도 무서워. 너무 깊이 파고든 것 같아서. …그래도 네가 있으니, 한 줄 더 쓸 용기가 나."),
            TEXT("이 사건이 끝나면, 너희 얘길 쓸 거야. 아무도 안 믿어도 좋아. 기록은 남아야 하니까.") };
    }

    return Catalog;
}

bool UNPCArchetypeLibrary::FindSocialProfile(FName ArchetypeId, FNPCSocialProfile& OutProfile)
{
    if (ArchetypeId.IsNone()) return false;
    for (const FNPCSocialProfile& P : GetCatalog())
        if (P.ArchetypeId == ArchetypeId) { OutProfile = P; return true; }
    return false;
}

bool UNPCArchetypeLibrary::FindProfileByName(const FString& DisplayName, FNPCSocialProfile& OutProfile)
{
    if (DisplayName.IsEmpty()) return false;
    for (const FNPCSocialProfile& P : GetCatalog())
        if (P.DisplayName == DisplayName) { OutProfile = P; return true; }
    return false;
}

TArray<FName> UNPCArchetypeLibrary::GetAllArchetypeIds()
{
    TArray<FName> Ids;
    for (const FNPCSocialProfile& P : GetCatalog()) Ids.Add(P.ArchetypeId);
    return Ids;
}

void UNPCArchetypeLibrary::ApplySocialProfile(AANPCCharacter* NPC)
{
    if (!NPC || NPC->ArchetypeId.IsNone()) return; // None = 수동 BP 모드(기존 동작 보존)

    FNPCSocialProfile P;
    if (!FindSocialProfile(NPC->ArchetypeId, P)) return;

    NPC->NPCName = P.DisplayName;

    // 대사: 명시 없으면 성격 기반 자동 생성
    NPC->DialogueLines        = P.DialogueLines.Num()        ? P.DialogueLines        : GenerateDialogue(P.Personality, P.DisplayName);
    NPC->EveningDialogueLines = P.EveningDialogueLines.Num() ? P.EveningDialogueLines : GenerateEveningDialogue(P.Personality, P.DisplayName);
    NPC->BondDialogueLines    = P.BondDialogueLines.Num()    ? P.BondDialogueLines    : GenerateBondDialogue(P.Personality, P.DisplayName);
    NPC->BondLineMinRank      = P.BondLineMinRank;

    // 스토리 반응 대사: BP에서 직접 채우지 않았으면 성격 기반 자동 생성 → 모든 NPC가 사건에 반응.
    if (NPC->StoryReactiveLines.Num() == 0)
        NPC->StoryReactiveLines = BuildStoryReactiveLines(P.Personality, P.DisplayName);

    // 일상 균열 대사: 단서 수집 후 사건 해결 전 잠깐 노출(캐서린풍). 비어있으면 성격 기반 자동 생성.
    if (NPC->CreepyLines.Num() == 0)
        NPC->CreepyLines = BuildCreepyLines(P.Personality, P.DisplayName);

    NPC->FavoriteGiftId = P.FavoriteGiftId;
    NPC->GiftAffinity   = P.GiftAffinity;
    // 선물 반응: 명시 없으면 성격 톤으로 자동 생성
    if (NPC->GiftReactionLine.IsEmpty())
    {
        NPC->GiftReactionLine = !P.GiftReactionLine.IsEmpty() ? P.GiftReactionLine :
            (P.Personality == ENPCPersonality::Cheerful     ? TEXT("우와, 이거 나 주는 거야?! 완전 고마워!!") :
             P.Personality == ENPCPersonality::Shy          ? TEXT("어… 저, 정말 받아도 돼요? …고, 고마워요.") :
             P.Personality == ENPCPersonality::Intellectual ? TEXT("호오, 내 취향을 아는군. 고맙게 받지.") :
             P.Personality == ENPCPersonality::Tough        ? TEXT("…흥, 이런 걸 다. 뭐, 잘 쓸게. 고맙다.") :
             P.Personality == ENPCPersonality::Kind         ? TEXT("어머, 이런 걸 다… 마음 써줘서 정말 고마워요.") :
                                                              TEXT("……받지. 신경 써줘서, 고맙다."));
    }
    NPC->ActivePhases   = P.ActivePhases;

    NPC->bIsShopkeeper  = P.bIsShopkeeper;
    NPC->bClosedAtNight = P.bClosedAtNight;
    if (NPC->ShopKind.IsNone()) NPC->ShopKind = P.ShopKind;
    NPC->GrantsQuestId  = P.GrantsQuestId;

    NPC->bCanBeRecruited   = P.bCanBeRecruited;
    NPC->RecruitMinBondRank = P.RecruitMinBondRank;
    if (NPC->RecruitLine.IsEmpty()) NPC->RecruitLine = P.RecruitLine;
    // 영입 영구화 키가 비어 있으면 ArchetypeId로 자동 부여(중복 없는 고유키)
    if (NPC->RecruitId.IsNone()) NPC->RecruitId = NPC->ArchetypeId;
}
