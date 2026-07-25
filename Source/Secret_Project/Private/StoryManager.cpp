#include "StoryManager.h"
#include "StatComponent.h"
#include "RelationshipComponent.h" // 인연 랭크 게이트(호출만)
#include "InventoryComponent.h"    // 스토리 아이템 보상(호출만)
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"         // GEngine->GetWorldFromContextObject
#include "Engine/GameInstance.h"   // GetSubsystem

// ── 조건 게이트(레벨 상태 스왑) ──────────────────────────
UStoryManagerSubsystem* UStoryManagerSubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject || !GEngine) return nullptr;
    if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull))
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            return GI->GetSubsystem<UStoryManagerSubsystem>();
        }
    }
    return nullptr;
}

bool UStoryManagerSubsystem::PassesFlagGate(const UObject* WorldContextObject, FName RequiredFlag, FName ForbiddenFlag)
{
    UStoryManagerSubsystem* S = Get(WorldContextObject);
    if (!S) return true; // 서브시스템 없으면 통과(안전 기본)
    if (!RequiredFlag.IsNone() && !S->HasFlag(RequiredFlag)) return false;
    if (!ForbiddenFlag.IsNone() && S->HasFlag(ForbiddenFlag)) return false;
    return true;
}

static const TCHAR* StorySlot() { return TEXT("StorySave"); }

// ── 메인 스토리 카탈로그 (현대 배경 + 페르소나풍) ──────────
// 항목 1개 = 메인 시나리오 한 장면. 추가 = 스토리 확장. 해금: 선행비트 + 날짜 + 플래그(보스 처치 등).
const TArray<FStoryBeat>& UStoryManagerSubsystem::GetCatalog()
{
    static TArray<FStoryBeat> Catalog;
    if (Catalog.Num() > 0) return Catalog;

    auto Beat = [](FName Id, int32 Chapter, const FString& Title) -> FStoryBeat&
    {
        FStoryBeat B; B.BeatId = Id; B.Chapter = Chapter; B.Title = Title;
        return Catalog[Catalog.Add(B)];
    };

    // ── 1장: 프롤로그(평범한 아침) ──
    {
        FStoryBeat& B = Beat(TEXT("b_prologue"), 1, TEXT("프롤로그 — 평범한 아침"));
        B.MinDay = 1;
        B.Lines = {
            { TEXT("나"),   TEXT("(또 똑같은 아침. 가방을 메고 집을 나선다.)") },
            { TEXT("나"),   TEXT("(……그런데 요즘 거리가, 어딘가 이상하다.)") },
            { TEXT("행인"), TEXT("어… 어라…") },
            { TEXT("나"),   TEXT("(앞서가던 사람이 갑자기 풀썩 주저앉는다. 눈빛이 텅 비었다. 다들 흘깃 보곤 그냥 지나친다.)") },
            { TEXT("나"),   TEXT("저기요, 괜찮으세요? ……안 들리시나.") },
            { TEXT("???"),  TEXT("그 사람은 못 들어. 이미 절반쯤, 저쪽으로 넘어갔으니까.") },
            { TEXT("나"),   TEXT("……누구야? 어디서 들리는 거지?") },
            { TEXT("???"),  TEXT("곧 알게 돼. 너는 — 아직 포기하지 않은 눈을 하고 있으니까.") },
        };
        B.GrantsFlag = TEXT("Prologue");
    }
    // ── 1장: 각성 ──
    {
        FStoryBeat& B = Beat(TEXT("b_awaken"), 1, TEXT("1장 — 각성"));
        B.PrereqBeatId = TEXT("b_prologue");
        B.Lines = {
            { TEXT("나"),     TEXT("(쓰러진 사람한테 다가간 순간 — 발밑이 출렁였다.)") },
            { TEXT("나"),     TEXT("뭐야 이거… 건물이, 거리가 통째로 휘어지고 있어.") },
            { TEXT("나"),     TEXT("(색이 다 빠진 거리. 그리고 저 끝에서, 뭔가가 기어 나온다.)") },
            { TEXT("???"),    TEXT("도망쳐도 따라잡혀. ……싸울 거야, 삼켜질 거야?") },
            { TEXT("나"),     TEXT("싸운다니, 저런 걸 어떻게—") },
            { TEXT("???"),    TEXT("이미 답은 정했잖아. 그러니 불러. 내 이름을.") },
            { TEXT("나"),     TEXT("(가슴 깊은 곳에서 무언가가 끓어오른다. 가면이… 깨진다.)") },
            { TEXT("페르소나"), TEXT("그래, 바로 그거다. 나는 또 다른 너 — 네가 부르는 한, 곁에 있겠다.") },
        };
        B.GrantsFlag = TEXT("Awakened");
    }
    // ── 1장: 첫 그림자 ──
    {
        FStoryBeat& B = Beat(TEXT("b_first_shadow"), 1, TEXT("1장 — 첫 그림자"));
        B.PrereqBeatId = TEXT("b_awaken");
        B.Lines = {
            { TEXT("나"),     TEXT("(손이 떨린다. 그래도… 여기서 등 돌리면, 저 사람은 영영 못 돌아와.)") },
            { TEXT("페르소나"), TEXT("겁먹는 건 당연해. 중요한 건, 그래도 발을 내딛느냐지.") },
            { TEXT("나"),     TEXT("……알겠어. 어디를 노리면 돼?") },
            { TEXT("페르소나"), TEXT("그것들에겐 약점이 있다. 거길 찌르면 무너뜨릴 수 있어 — 그 틈을 놓치지 마.") },
            { TEXT("나"),     TEXT("약점이라. ……좋아, 해보자!") },
        };
        B.GrantsFlag = TEXT("FirstShadowCleared");
        B.RewardGold = 100;
        B.RewardItemId = TEXT("HealPotion"); B.RewardItemCount = 2;
    }
    // ── 2장: 동료 ──
    {
        FStoryBeat& B = Beat(TEXT("b_allies"), 2, TEXT("2장 — 혼자가 아니야"));
        B.PrereqBeatId = TEXT("b_first_shadow");
        B.MinDay = 3;
        B.Lines = {
            { TEXT("강현"), TEXT("너지. 어제 그 일그러진 거리에서, 가면 쓰고 싸우던 놈.") },
            { TEXT("나"),  TEXT("……너도 봤어? 그럼 너도—") },
            { TEXT("강현"), TEXT("어. 나도 그 '힘'이 튀어나오더라고. 솔직히 아직도 뭐가 뭔지 모르겠다만.") },
            { TEXT("서연"), TEXT("저… 저기. 두 분도, 그… 그림자가 보이는 거죠?") },
            { TEXT("강현"), TEXT("어라, 도서부 너도냐?") },
            { TEXT("서연"), TEXT("혼자선 무서워서… 계속 모른 척했어요. 근데 더는, 못 본 척 못 하겠어서.") },
            { TEXT("나"),  TEXT("……같은 걸 본 사람이 나만은 아니었구나.") },
            { TEXT("나"),  TEXT("(셋이 모이니, 어제의 공포가 조금은 견딜 만해진다.) 같이 하자. 우리라면, 어떻게든 될 것 같아.") },
        };
        B.GrantsFlag = TEXT("TeamFormed");
    }
    // ── 3장: 흉흉한 거리 ──
    {
        FStoryBeat& B = Beat(TEXT("b_unrest"), 3, TEXT("3장 — 흉흉한 거리"));
        B.PrereqBeatId = TEXT("b_allies");
        B.MinDay = 6;
        B.Lines = {
            { TEXT("윤 기자"), TEXT("쓰러지는 사람이 일주일 새 세 배로 늘었어. 병원도 원인을 못 찾고.") },
            { TEXT("나"),     TEXT("기자님은… 이게 그냥 병이 아니란 걸, 알고 계신 거죠?") },
            { TEXT("윤 기자"), TEXT("심증뿐이야. 그래서 너희한테 흘리는 거고. ……이 사람한테도.") },
            { TEXT("탐정 진"), TEXT("뒤에서 그림자를 부리는 자가 있어. 거리에선 '흑마술사'라고들 부르더군.") },
            { TEXT("나"),     TEXT("그게 사람들 마음을 거둬가는 범인이라는 거야?") },
            { TEXT("탐정 진"), TEXT("아직은 가설이야. 확인하려면, 직접 찾아가는 수밖에.") },
        };
        B.GrantsFlag = TEXT("WarlockRumor");
    }
    // ── 3장: 추적 ──
    {
        FStoryBeat& B = Beat(TEXT("b_warlock_search"), 3, TEXT("3장 — 추적"));
        B.PrereqBeatId = TEXT("b_unrest");
        B.MinDay = 8;
        B.Lines = {
            { TEXT("탐정 진"), TEXT("흑마술사의 거점을 좁혔어. 폐건물 지하… 그림자가 들끓는 곳이지.") },
            { TEXT("강현"),   TEXT("좋아, 쳐들어가자. 더 망설이면 또 누가 당해.") },
            { TEXT("나"),     TEXT("각자 약점을 분담하자. 우리 팀이라면, 할 수 있어.") },
        };
        B.GrantsFlag = TEXT("WarlockLocated");
        B.RewardGold = 150;
    }
    // ── 4장: 흑마술사 (보스1 처치로 해금) ──
    {
        FStoryBeat& B = Beat(TEXT("b_warlock_fall"), 4, TEXT("4장 — 거짓의 가면"));
        B.PrereqBeatId = TEXT("b_warlock_search");
        B.RequiredFlag = TEXT("shadow_warlock"); // 흑마술사 보스 처치 시 전투가 세우는 플래그
        B.Lines = {
            { TEXT("흑마술사"), TEXT("크윽… 고작 이런 애송이들에게…!") },
            { TEXT("흑마술사"), TEXT("하지만 늦었어. 군림자님의 강림은… 이미 시작됐다…!") },
            { TEXT("탐정 진"),  TEXT("끝이 아니야. 이 자도… 누군가에게 조종당한 끄나풀에 불과해.") },
            { TEXT("은우"),    TEXT("데이터 추적했어. 신호의 끝에… 더 거대한 그림자가 있어.") },
            { TEXT("나"),      TEXT("진짜 '군림자'가 따로 있다는 건가. ……끝까지 간다.") },
        };
        B.GrantsFlag = TEXT("WarlockDefeated");
        B.RewardGold = 300;
        B.RewardItemId = TEXT("HiPotion"); B.RewardItemCount = 2;
    }
    // ── 5장: 결전 전야 ──
    {
        FStoryBeat& B = Beat(TEXT("b_resolve"), 5, TEXT("5장 — 결전 전야"));
        B.PrereqBeatId = TEXT("b_warlock_fall");
        B.MinDay = 12;
        B.Lines = {
            { TEXT("서연"),   TEXT("진짜 배후가… '군림자'라는 그자라니. 무서워요. 그치만…") },
            { TEXT("민지 쌤"), TEXT("다들 무리하지 마. 살아서 돌아오는 게 제일 중요하니까.") },
            { TEXT("나"),     TEXT("여기까지 함께 왔잖아. 내일, 끝을 보자.") },
        };
        B.GrantsFlag = TEXT("Resolved");
    }
    // ── 5장: 군림자의 진실 (보스2 처치로 해금) ──
    {
        FStoryBeat& B = Beat(TEXT("b_tyrant_truth"), 5, TEXT("5장 — 군림자의 진실"));
        B.PrereqBeatId = TEXT("b_resolve");
        B.RequiredFlag = TEXT("shadow_tyrant"); // 군림하는 그림자 처치 시 플래그
        B.Lines = {
            { TEXT("군림자"), TEXT("여기까지 온 건 칭찬해 주마. 하지만 끝이다.") },
            { TEXT("군림자"), TEXT("인간은 약해. 그래서 내가 '군림'해 다스려 주는 것이다. 그게 질서야.") },
            { TEXT("강현"),   TEXT("웃기지 마. 약하면 다스려져야 한다고? 그딴 질서, 우리가 부순다.") },
            { TEXT("서연"),   TEXT("저… 예전 같았으면 도망쳤을 거예요. 근데 지금은, 안 물러서요!") },
            { TEXT("나"),    TEXT("약하니까 서로 기댄다. 그게 네가 끝내 몰랐던 거야.") },
            { TEXT("군림자"), TEXT("…그 빛은, 대체 뭐지. 어째서 꺼지지 않는…!") },
            { TEXT("페르소나"), TEXT("이 유대가, 네 진짜 힘이다. ——가라!") },
            { TEXT("나"),    TEXT("끝내자. 모두의 내일을 위해서!") },
        };
        B.GrantsFlag = TEXT("TyrantDefeated");
        B.RewardGold = 500;
        B.RewardItemId = TEXT("Elixir"); B.RewardItemCount = 2;
    }
    // ── 에필로그 ──
    {
        FStoryBeat& B = Beat(TEXT("b_finale"), 5, TEXT("에필로그 — 일상으로"));
        B.PrereqBeatId = TEXT("b_tyrant_truth");
        B.MinDay = 14;
        B.Lines = {
            { TEXT("나"),     TEXT("거리는 다시 평범한 아침을 맞았다. 그림자도, 일그러진 하늘도 없는.") },
            { TEXT("탐정 진"), TEXT("사건은 종결. ……이지만, 가끔은 이렇게 다 같이 모이자고.") },
            { TEXT("민지 쌤"), TEXT("다친 데 없이 다 돌아와서 다행이야. 정말, 다행이야.") },
            { TEXT("강현"),   TEXT("뭐야 이 분위기, 간지럽게. ……그래도 나쁘진 않네.") },
            { TEXT("나"),     TEXT("특별할 것 없는 하루. 하지만 이젠 안다 — 곁에 누가 있는지.") },
            { TEXT("페르소나"), TEXT("잘 싸웠다. 언제든, 다시 부르면 된다.") },
        };
        B.GrantsFlag = TEXT("StoryClear");
    }

    // ── 인터루드 (빌런 POV · 일상) — 메인 사이 자동 등장(보스/인연 게이트 없음) ──
    {
        FStoryBeat& B = Beat(TEXT("b_il_villain1"), 2, TEXT("막간 — 어둠의 옥좌"));
        B.PrereqBeatId = TEXT("b_allies");
        B.MinDay = 4;
        B.Lines = {
            { TEXT("???"),    TEXT("(아무도 없는 옥좌. 그림자들이 무릎 꿇는다.)") },
            { TEXT("군림자"), TEXT("벌써 깨어난 아이가 있다고? …흥미롭군. 하지만 부질없지.") },
            { TEXT("군림자"), TEXT("인간은 결국 서로를 배신해. 그 '유대'라는 환상째로, 짓밟아주마.") },
        };
        B.GrantsFlag = TEXT("Saw_Villain1");
    }
    {
        FStoryBeat& B = Beat(TEXT("b_il_school"), 2, TEXT("막간 — 방과 후"));
        B.PrereqBeatId = TEXT("b_allies");
        B.MinDay = 5;
        B.Lines = {
            { TEXT("강현"), TEXT("야, 맨날 싸움 얘기만 하지 말고 가끔은 분식이라도 먹자고.") },
            { TEXT("서연"), TEXT("어… 저도, 같이 가도 될까요?") },
            { TEXT("나"),  TEXT("(이런 평범한 시간이, 우리가 지키려는 거 그 자체였다.)") },
        };
        B.GrantsFlag = TEXT("AfterSchool");
    }
    {
        FStoryBeat& B = Beat(TEXT("b_il_villain2"), 3, TEXT("막간 — 끄나풀"));
        B.PrereqBeatId = TEXT("b_unrest");
        B.MinDay = 7;
        B.Lines = {
            { TEXT("흑마술사"), TEXT("군림자님, 그 애송이들… 제가 처리하겠습니다. 부디 기회를.") },
            { TEXT("군림자"),  TEXT("실패하면 너도 그림자다. 가라, 가면을 쓴 자여.") },
        };
        B.GrantsFlag = TEXT("Saw_Villain2");
    }
    {
        FStoryBeat& B = Beat(TEXT("b_il_villain3"), 4, TEXT("막간 — 균열"));
        B.PrereqBeatId = TEXT("b_warlock_fall");
        B.Lines = {
            { TEXT("군림자"), TEXT("끄나풀이 당했나. …그 유대, 생각보다 단단하군.") },
            { TEXT("군림자"), TEXT("좋다. 직접 부숴주지. 절망이 무엇인지, 똑똑히 가르쳐주마.") },
        };
        B.GrantsFlag = TEXT("Saw_Villain3");
    }
    // ── 히든 트루 엔딩 (StoryClear + 핵심 인연 MAX 3인) ──
    {
        FStoryBeat& B = Beat(TEXT("b_true_ending"), 6, TEXT("진 엔딩 — 끊기지 않는 인연"));
        B.PrereqBeatId = TEXT("b_finale");
        B.RequiredAllFlags = {
            TEXT("Bond_bond_kanghyun_max"),
            TEXT("Bond_bond_detective_max"),
            TEXT("Bond_bond_eunwoo_max"),
        };
        B.Lines = {
            { TEXT("나"),     TEXT("군림자는 사라졌지만, 그가 남긴 말이 가시처럼 남았다. '인간은 배신한다'고.") },
            { TEXT("강현"),   TEXT("증명해줬잖아. 우린 끝까지 안 놨어.") },
            { TEXT("탐정 진"), TEXT("이 유대는 사건이 끝나도 남아. 영구 미제가 아니라… 영구 동행이지.") },
            { TEXT("은우"),   TEXT("연결 안 끊겨. 내가 보증해. 평생 핑 보낼 거니까 각오해.") },
            { TEXT("페르소나"), TEXT("이것이 네 답이다. 약하기에, 함께 강해진 자들의.") },
            { TEXT("나"),     TEXT("(새로운 아침. 그리고 우리는, 여전히 함께다.) — 끝") },
        };
        B.GrantsFlag = TEXT("TrueEnding");
        B.RewardGold = 1000;
        B.RewardItemId = TEXT("Elixir"); B.RewardItemCount = 5;
    }

    // ── 미스터리/심리 호러 드립 (Chapter -1) — 일상 사이 날짜 게이트로 등장, 단서 수집 ──
    // 우선순위: 메인 > 인터루드 > 미스터리 > 인연(카탈로그 순). 캐서린풍: 낮 괴담 호기심 → 점층 공포 → 진실 수렴.
    auto Mystery = [&](FName Id, const FString& Title, int32 Day, FName Grants,
                       const TArray<FStoryLine>& Lines) -> FStoryBeat&
    {
        FStoryBeat& B = Beat(Id, -1, Title);
        B.MinDay = Day;
        B.GrantsFlag = Grants;
        B.Lines = Lines;
        return B;
    };

    Mystery(TEXT("m_rumor_mirror"), TEXT("괴담 — 거울 너머의 나"), 2, TEXT("Clue_Mirror"), {
        { TEXT("반 친구"), TEXT("야, 그거 들었어? 밤에 거울 오래 들여다보면… 비친 내가 딴짓 한대.") },
        { TEXT("반 친구"), TEXT("반박자 늦게 웃거나, 안 한 표정을 짓는다나. 으~ 소름.") },
        { TEXT("나"),     TEXT("(괜한 소리… 인데, 어젯밤 거울 속 내가 정말 한 박자 늦게 웃지 않았나?)") },
    });
    Mystery(TEXT("m_nightmare1"), TEXT("악몽 — 무릎 꿇은 거리"), 3, TEXT("Clue_Nightmare1"), {
        { TEXT("…"),    TEXT("(안개 속. 수많은 사람들이 무릎을 꿇고 있다. 저 멀리, 무언가가 빛나는 옥좌.)") },
        { TEXT("???"),  TEXT("편해지고 싶지 않아? 다 내려놓으면… 아주 편한데.") },
        { TEXT("나"),   TEXT("(식은땀에 잠이 깼다. 창밖은 아직 깜깜하다. …방금 그 목소리, 누구지.)") },
    });
    Mystery(TEXT("m_wrongness1"), TEXT("균열 — 점장님이 이상하다"), 5, TEXT("Clue_Wrongness"), {
        { TEXT("점장 태수"), TEXT("어서 와~ ……너도 곧, 편해질 거야.") },
        { TEXT("나"),       TEXT("…네? 방금 뭐라고 하셨어요?") },
        { TEXT("점장 태수"), TEXT("응? 내가 뭐랬나? 하하, 요즘 통 잠을 못 자서. 신경 쓰지 마.") },
        { TEXT("나"),       TEXT("(분명, 내가 모르는 말투였어. 잠깐 다른 사람 같았다.)") },
    });
    Mystery(TEXT("m_rumor_lastcar"), TEXT("괴담 — 공허행 막차"), 6, TEXT("Clue_Hollow"), {
        { TEXT("서연"), TEXT("막차에서 졸면 안 된대요. 다음 날… 눈빛이 텅 빈 채로 깬다고.") },
        { TEXT("서연"), TEXT("요즘 그 '공허 현상' 있잖아요. 괴담인 줄 알았는데… 너무 비슷해서.") },
        { TEXT("나"),  TEXT("(괴담이 아니라, 누군가 실제로 겪은 일을 옮긴 거라면.)") },
    });
    Mystery(TEXT("m_blackmsg"), TEXT("균열 — 검은 메시지"), 7, TEXT("Clue_Voice"), {
        { TEXT("(휴대폰)"), TEXT("[모르는 번호] 넌 약해. 그러니까 다 내려놔. 내가 편하게 해줄게.") },
        { TEXT("나"),      TEXT("(누가 보낸 거지? …삭제했는데.)") },
        { TEXT("(휴대폰)"), TEXT("[모르는 번호] 삭제해도 소용없어. 너도 결국, 무릎 꿇게 될 테니까.") },
        { TEXT("나"),      TEXT("(악몽 속 그 목소리와… 똑같다.)") },
    });
    Mystery(TEXT("m_nightmare2"), TEXT("악몽 — 또렷해지는 옥좌"), 9, TEXT("Clue_Throne"), {
        { TEXT("…"),       TEXT("(또 그 꿈. 옥좌의 형체가 더 또렷하다. 무릎 꿇은 군중 사이로, 낯익은 얼굴 — 며칠 전 쓰러졌던 그 사람.)") },
        { TEXT("군중"),    TEXT("편하다… 편하다… 편하다…") },
        { TEXT("???"),     TEXT("너도, 곧.") },
        { TEXT("나"),      TEXT("(이건 단순한 꿈이 아니야. 무언가가… 나를 부르고 있어.)") },
    }).PrereqBeatId = TEXT("m_nightmare1");
    Mystery(TEXT("m_forgotten"), TEXT("괴담 — 잊힌 자리"), 10, TEXT("Clue_Forgotten"), {
        { TEXT("강현"), TEXT("어? 저기 창가 자리… 원래 누구 앉지 않았나?") },
        { TEXT("서연"), TEXT("…글쎄요. 처음부터 비어 있던 것 같은데요.") },
        { TEXT("나"),  TEXT("(분명 누가 있었다. 웃고, 떠들던 사람이. 근데 이름도, 얼굴도… 기억이 안 나.)") },
        { TEXT("나"),  TEXT("(완전히 먹힌 거다. 존재 자체가, 모두의 기억에서.)") },
    });
    // 단서 4개 모두 모이면 진실의 윤곽
    {
        FStoryBeat& B = Beat(TEXT("m_truth_glimpse"), -1, TEXT("진실의 윤곽 — 범인은 누구도 아니다"));
        B.MinDay = 11;
        B.RequiredAllFlags = { TEXT("Clue_Mirror"), TEXT("Clue_Hollow"), TEXT("Clue_Voice"), TEXT("Clue_Throne") };
        B.Lines = {
            { TEXT("나"),     TEXT("(거울, 막차, 문자, 악몽… 흩어진 조각이 하나를 가리킨다.)") },
            { TEXT("나"),     TEXT("(범인은 '누군가'가 아니야. 사람들이 스스로 '내려놓고 싶다'고 비는 그 마음… 그게 모이고 있어.)") },
            { TEXT("페르소나"), TEXT("그래. 그 체념이 응결해, 군림하려는 것이다. 외부의 적이 아니다 — 너희 안의 포기다.") },
            { TEXT("나"),     TEXT("(등골이 서늘하다. 가장 무서운 건… 괴물이 아니었어.)") },
        };
        B.GrantsFlag = TEXT("Clue_Truth");
        B.RewardGold = 100;
    }
    Mystery(TEXT("m_rumor_clocktower"), TEXT("괴담 — 멈추는 시계탑"), 4, TEXT("Clue_Clock"), {
        { TEXT("지훈"), TEXT("한울 시계탑 말야, 가끔 딱 1분 멈춘대. 그 1분 동안은… '딴 세상'이 겹친다나.") },
        { TEXT("나"),  TEXT("(이면. 현실과 그 세계가 겹치는 순간을, 사람들은 '멈춘 시계'로 느끼는 거야.)") },
    });
    Mystery(TEXT("m_rumor_rooftop"), TEXT("괴담 — 옥상의 가면"), 8, TEXT("Clue_Mask"), {
        { TEXT("DJ 제이"), TEXT("야자 끝나고 옥상 봤는데… 가면 쓴 누가 서 있더라. 눈 깜빡이니까 없어졌어.") },
        { TEXT("나"),     TEXT("(가면… 악몽 속 옥좌의 그것과는 달라. 더 가깝고, 더 인간 같은.)") },
        { TEXT("나"),     TEXT("(저게 사람들의 마음을 거두는 '술사'라면 — 직접 마주해야 한다.)") },
    }).PrereqBeatId = TEXT("b_unrest");
    Mystery(TEXT("m_sheep"), TEXT("괴담 — 양을 세지 마라"), 8, TEXT("Clue_Sheep"), {
        { TEXT("바텐더 레이"), TEXT("잠 안 온다고? ……양은 세지 마. 셋까지 세면, '그분'이 한 마리씩 데려간다더라.") },
        { TEXT("바텐더 레이"), TEXT("농담이야. ……근데 요즘, 안 깨어나는 손님이 늘긴 했어.") },
        { TEXT("나"),         TEXT("(웃어넘기려는데, 등이 서늘하다.)") },
    });
    // 악몽 3차 — 진실을 안 뒤, 결전 직전. 유대가 악몽을 깬다(트루엔딩 복선).
    {
        FStoryBeat& B = Beat(TEXT("m_nightmare3"), -1, TEXT("악몽 — 내미는 손"));
        B.MinDay = 13;
        B.RequiredFlag = TEXT("Clue_Truth");
        B.Lines = {
            { TEXT("…"),       TEXT("(또 그 거리. 이번엔 내 무릎이 저절로 꺾인다. 편하다. 내려놓으면, 편하다—)") },
            { TEXT("군림자"), TEXT("그래. 너도 결국 똑같아. 인간은 약하니까.") },
            { TEXT("강현"),   TEXT("(멀리서) 야! 어디서 무릎을 꿇어, 일어나!") },
            { TEXT("서연"),   TEXT("(손을 내밀며) 혼자 두지 않을게요. 같이… 일어나요.") },
            { TEXT("나"),     TEXT("(잡았다. 따뜻하다. 악몽이 깨진다.) ——약해서, 같이 일어서는 거다.") },
        };
        B.GrantsFlag = TEXT("NightmareOvercome");
        B.RewardGold = 150;
    }
    // 팀이 진실을 공유 — 공포를 결의로
    {
        FStoryBeat& B = Beat(TEXT("m_team_dread"), -1, TEXT("진실 공유 — 가장 무서운 것"));
        B.RequiredFlag = TEXT("Clue_Truth");
        B.RequiredAllFlags = { TEXT("TeamFormed") };
        B.Lines = {
            { TEXT("나"),     TEXT("범인은 누구도 아니야. 사람들이 스스로 포기하는 마음… 그게 군림자야.") },
            { TEXT("탐정 진"), TEXT("…최악의 진실이군. 싸울 적이 '우리 안'에 있다는 거니까.") },
            { TEXT("은우"),   TEXT("그럼 못 이기는 거 아냐? 사람들이 계속 포기하면…") },
            { TEXT("강현"),   TEXT("아니. 여기 안 포기한 놈들이 있잖아. 우리가 증거야.") },
            { TEXT("나"),     TEXT("맞아. 유대는 환상이 아니라는 걸, 그놈한테 똑똑히 보여주자.") },
        };
        B.GrantsFlag = TEXT("DreadShared");
        B.RewardGold = 120;
    }

    // ── 인연(소셜링크) 에피소드 ──────────────────────────
    // B RelationshipComponent 랭크 3 도달 시 해금(+TeamFormed). 메인 비트 뒤에 배치 = 메인 우선.
    // 캐릭터 깊이 + 보상. 확장 = 여기 비트 추가(랭크 5/7/10 후속 에피소드 등).
    auto Bond = [&](FName Id, const FString& NpcName, const FString& Title, int32 Rank,
                    const TArray<FStoryLine>& Lines) -> FStoryBeat&
    {
        FStoryBeat& B = Beat(Id, 0, Title); // Chapter 0 = 인연(메인 챕터 진척에 영향 없음)
        B.RequiredFlag = TEXT("TeamFormed");
        B.RequiredBondNPC = FName(*NpcName);
        B.RequiredBondRank = Rank;
        B.Lines = Lines;
        B.GrantsFlag = FName(*FString::Printf(TEXT("Bond_%s"), *Id.ToString()));
        B.RewardGold = 80;
        return B;
    };

    Bond(TEXT("bond_kanghyun"), TEXT("강현"), TEXT("인연 — 강현: 주먹의 이유"), 3, {
        { TEXT("강현"), TEXT("난 약한 게 싫어. 약하면… 지키고 싶은 걸 못 지키니까.") },
        { TEXT("강현"), TEXT("너랑 같이 싸우면, 내가 더 강해지는 기분이야. 등 맡길게.") },
    });
    Bond(TEXT("bond_eunwoo"), TEXT("은우"), TEXT("인연 — 은우: 화면 너머"), 3, {
        { TEXT("은우"), TEXT("난 사람보다 데이터가 편했어. 거짓말을 안 하거든.") },
        { TEXT("은우"), TEXT("근데 너넨… 좀 다르더라. 같이 있어도 괜찮은 노이즈랄까.") },
    });
    Bond(TEXT("bond_detective"), TEXT("탐정 진"), TEXT("인연 — 탐정 진: 미해결"), 3, {
        { TEXT("탐정 진"), TEXT("……내가 못 풀었던 사건이 하나 있어. 그 그림자와 무관하지 않지.") },
        { TEXT("탐정 진"), TEXT("네 덕에 다시 추적할 용기가 생겼다. ……고맙다, 진심으로.") },
    });
    Bond(TEXT("bond_minji"), TEXT("민지 쌤"), TEXT("인연 — 민지 쌤: 보건실의 불빛"), 3, {
        { TEXT("민지 쌤"), TEXT("다친 애들 치료해 주다 보면, 정작 내 상처는 못 보더라고.") },
        { TEXT("민지 쌤"), TEXT("…너는 가끔 내 안부도 물어봐 주네. 그게 참, 고마워.") },
    });
    Bond(TEXT("bond_seoyeon"), TEXT("서연"), TEXT("인연 — 서연: 책 밖의 세상"), 3, {
        { TEXT("서연"), TEXT("저는… 책 속에 숨는 게 편했어요. 현실은 너무 무서워서.") },
        { TEXT("서연"), TEXT("그런데 당신과 있으면, 한 페이지쯤 더 넘겨볼 용기가 나요.") },
    });
    Bond(TEXT("bond_dohyun"), TEXT("학생회장 도현"), TEXT("인연 — 도현: 모두의 무게"), 3, {
        { TEXT("학생회장 도현"), TEXT("리더는 약한 모습을 보이면 안 돼. ……라고 믿어 왔지.") },
        { TEXT("학생회장 도현"), TEXT("네 앞에선 잠깐, 그냥 도현이어도 될 것 같아.") },
    });
    Bond(TEXT("bond_luna"), TEXT("연습생 루나"), TEXT("인연 — 루나: 무대의 꿈"), 3, {
        { TEXT("연습생 루나"), TEXT("데뷔는 멀고, 연습은 끝이 없고… 가끔 다 놓고 싶어져.") },
        { TEXT("연습생 루나"), TEXT("그래도 네가 응원해 주면, 한 번 더 무대에 설 수 있을 것 같아!") },
    });
    Bond(TEXT("bond_zero"), TEXT("프로게이머 제로"), TEXT("인연 — 제로: 마지막 라운드"), 3, {
        { TEXT("프로게이머 제로"), TEXT("이기는 것만 생각했어. 지면 아무도 안 봐주니까.") },
        { TEXT("프로게이머 제로"), TEXT("근데 너랑 한 팀이 되고 나선… 지는 것도 좀 덜 무섭더라.") },
    });
    // ── 인연 에피소드: 남은 영입 캐릭터 (랭크3) ──
    Bond(TEXT("bond_jihun"), TEXT("지훈"), TEXT("인연 — 지훈: 같이 달리자"), 3, {
        { TEXT("지훈"), TEXT("야! 표정이 왜 그래~ 힘들면 같이 한 바퀴 뛰자, 다 풀려!") },
        { TEXT("지훈"), TEXT("난 네가 웃는 게 좋더라. 우리, 끝까지 같이 가는 거다?") },
    });
    Bond(TEXT("bond_bora"), TEXT("보라"), TEXT("인연 — 보라: 뒤에서 미는 힘"), 3, {
        { TEXT("보라"), TEXT("매니저는 늘 뒤에서 챙기는 자리야. 근데 그게 싫진 않아.") },
        { TEXT("보라"), TEXT("네가 앞에서 싸울 때, 내가 뒤를 받칠게. 그게 내 역할이니까!") },
    });
    Bond(TEXT("bond_cheolmin"), TEXT("라이더 철민"), TEXT("인연 — 철민: 바람의 끝"), 3, {
        { TEXT("라이더 철민"), TEXT("바이크로 달리면 다 잊혀져. 근데 도망치는 거랑은 다르더라고.") },
        { TEXT("라이더 철민"), TEXT("너랑 다니면서 알았어. 돌아올 곳이 있으니까 더 멀리 갈 수 있는 거다.") },
    });
    Bond(TEXT("bond_seonwoo"), TEXT("수의사 선우"), TEXT("인연 — 선우: 말 못 하는 것들"), 3, {
        { TEXT("수의사 선우"), TEXT("동물은 아프다고 말을 못 해. 그래서 더 잘 봐줘야 하지.") },
        { TEXT("수의사 선우"), TEXT("사람도 똑같더라. …너도, 혼자 참지 말고 말해줘.") },
    });
    Bond(TEXT("bond_gangcop"), TEXT("강 형사"), TEXT("인연 — 강 형사: 배지의 무게"), 3, {
        { TEXT("강 형사"), TEXT("법으로 못 잡는 것들이 있어. 그게 형사로서 제일 분하지.") },
        { TEXT("강 형사"), TEXT("…네 방식이라면, 그걸 잡을 수 있을지도 모르겠군. 함께 가지.") },
    });
    Bond(TEXT("bond_jeong"), TEXT("정 선생님"), TEXT("인연 — 정 선생님: 살리는 손"), 3, {
        { TEXT("정 선생님"), TEXT("의사가 돼서도, 못 살린 사람이 자꾸 떠올라.") },
        { TEXT("정 선생님"), TEXT("그래도 네 곁에서 한 명이라도 더 지킬 수 있다면… 의미가 있겠지.") },
    });
    // ── 인연 심화 에피소드 (랭크5, 랭크3 에피소드 선행) ──
    Bond(TEXT("bond_kanghyun_2"), TEXT("강현"), TEXT("인연 — 강현: 진짜 강함"), 5, {
        { TEXT("강현"), TEXT("예전엔 센 주먹이 강함인 줄 알았어. 지금은 아니야.") },
        { TEXT("강현"), TEXT("지킬 게 있고, 같이 지켜줄 녀석이 있는 거. 그게 진짜 강한 거더라.") },
    }).PrereqBeatId = TEXT("bond_kanghyun");
    Bond(TEXT("bond_eunwoo_2"), TEXT("은우"), TEXT("인연 — 은우: 로그아웃"), 5, {
        { TEXT("은우"), TEXT("난 늘 화면 뒤에 숨어 살았어. 근데 요즘은 밖이 더 재밌더라.") },
        { TEXT("은우"), TEXT("너희가 내 진짜 '접속'이야. …이런 말 오글거리니까 한 번만 할게.") },
    }).PrereqBeatId = TEXT("bond_eunwoo");
    Bond(TEXT("bond_detective_2"), TEXT("탐정 진"), TEXT("인연 — 탐정 진: 사건 종결"), 5, {
        { TEXT("탐정 진"), TEXT("그 미해결 사건… 드디어 마침표를 찍었다. 네 덕분이야.") },
        { TEXT("탐정 진"), TEXT("다음 사건이 와도 두렵지 않아. 믿을 파트너가 생겼으니까.") },
    }).PrereqBeatId = TEXT("bond_detective");

    // ── 인연 심화 (랭크7) — 약점/과거를 털어놓는 전환점 ──
    Bond(TEXT("bond_jihun_3"), TEXT("지훈"), TEXT("인연 — 지훈: 웃음 뒤"), 7, {
        { TEXT("지훈"), TEXT("나 항상 밝지? …사실은, 분위기 가라앉는 게 무서워서 그래.") },
        { TEXT("지훈"), TEXT("근데 너한텐 안 꾸며도 되더라. 처음이야, 그런 거.") },
    });
    Bond(TEXT("bond_seoyeon_3"), TEXT("서연"), TEXT("인연 — 서연: 첫 문장"), 7, {
        { TEXT("서연"), TEXT("저… 사실 소설을 써요. 아무한테도 안 보여줬는데.") },
        { TEXT("서연"), TEXT("당신 얘기를 쓰고 싶어졌어요. 용감한 주인공으로요.") },
    });
    Bond(TEXT("bond_kanghyun_3"), TEXT("강현"), TEXT("인연 — 강현: 그날의 패배"), 7, {
        { TEXT("강현"), TEXT("중학교 때 시합서 크게 졌어. 그 뒤로 지는 게 죽기보다 싫었지.") },
        { TEXT("강현"), TEXT("근데 너랑 깨지고 다시 일어나 보니까… 지는 것도 과정이더라.") },
    });
    Bond(TEXT("bond_minji_3"), TEXT("민지 쌤"), TEXT("인연 — 민지 쌤: 텅 빈 보건실"), 7, {
        { TEXT("민지 쌤"), TEXT("솔직히… 요즘 다 그만두고 싶었어. 아무도 날 안 챙기는 것 같아서.") },
        { TEXT("민지 쌤"), TEXT("근데 네가 매일 들러주잖아. 그거 하나로 버텨졌어. 정말로.") },
    });
    Bond(TEXT("bond_detective_3"), TEXT("탐정 진"), TEXT("인연 — 탐정 진: 혼자의 끝"), 7, {
        { TEXT("탐정 진"), TEXT("난 늘 혼자 일했어. 누굴 믿었다 다친 적이 있어서.") },
        { TEXT("탐정 진"), TEXT("…그 원칙, 네 앞에서만 접어두기로 했다.") },
    });
    Bond(TEXT("bond_eunwoo_3"), TEXT("은우"), TEXT("인연 — 은우: 닉네임 말고"), 7, {
        { TEXT("은우"), TEXT("온라인에선 다들 날 대단하다 했어. 근데 정작 내 이름은 몰라.") },
        { TEXT("은우"), TEXT("너는 '은우'를 봐주잖아. 그게… 좀 이상하게 좋아.") },
    });
    Bond(TEXT("bond_bora_3"), TEXT("보라"), TEXT("인연 — 보라: 내 차례"), 7, {
        { TEXT("보라"), TEXT("늘 남 챙기다 보니, 내가 뭘 좋아했는지도 까먹었더라.") },
        { TEXT("보라"), TEXT("네가 '넌 뭐 하고 싶냐'고 물어준 거, 처음이었어. 고마워.") },
    });
    Bond(TEXT("bond_dohyun_3"), TEXT("학생회장 도현"), TEXT("인연 — 도현: 완벽이라는 감옥"), 7, {
        { TEXT("학생회장 도현"), TEXT("모두가 기대하는 '완벽한 회장'… 그게 가끔 숨이 막혀.") },
        { TEXT("학생회장 도현"), TEXT("네 앞에선 실수해도 될 것 같아. 그게 얼마나 큰 건지, 너는 모를 거야.") },
    });
    Bond(TEXT("bond_cheolmin_3"), TEXT("라이더 철민"), TEXT("인연 — 철민: 멈춰 설 곳"), 7, {
        { TEXT("라이더 철민"), TEXT("계속 달리기만 했어. 멈추면 뭔가 따라잡힐 것 같아서.") },
        { TEXT("라이더 철민"), TEXT("근데 너랑 있으면, 잠깐 시동 꺼도 괜찮더라.") },
    });
    Bond(TEXT("bond_seonwoo_3"), TEXT("수의사 선우"), TEXT("인연 — 선우: 못 보낸 아이"), 7, {
        { TEXT("수의사 선우"), TEXT("못 살린 아이가 하나 있어. 아직도 그 눈빛이 떠올라.") },
        { TEXT("수의사 선우"), TEXT("…네가 들어줘서, 오늘은 조금 가벼워졌어.") },
    });
    Bond(TEXT("bond_luna_3"), TEXT("연습생 루나"), TEXT("인연 — 루나: 무대 뒤 눈물"), 7, {
        { TEXT("연습생 루나"), TEXT("동기는 벌써 데뷔했는데… 난 아직이야. 웃고 있지만 사실 초조해.") },
        { TEXT("연습생 루나"), TEXT("그래도 네가 '넌 빛난다'고 해주면, 한 번 더 해볼 수 있어.") },
    });
    Bond(TEXT("bond_gangcop_3"), TEXT("강 형사"), TEXT("인연 — 강 형사: 규칙과 진실"), 7, {
        { TEXT("강 형사"), TEXT("규칙을 지키느라 놓친 진실이 있어. 그게 형사로서 평생의 짐이지.") },
        { TEXT("강 형사"), TEXT("네 방식은 위험하지만… 가끔은 그게 옳더군. 배운다, 너한테.") },
    });
    Bond(TEXT("bond_jeong_3"), TEXT("정 선생님"), TEXT("인연 — 정 선생님: 신이 아니라서"), 7, {
        { TEXT("정 선생님"), TEXT("의사도 사람이라, 모두를 살리진 못해. 그 무력함이 가끔 무서워.") },
        { TEXT("정 선생님"), TEXT("그래도 네 곁이라면, 포기하지 않을 수 있을 것 같아.") },
    });
    Bond(TEXT("bond_zero_3"), TEXT("프로게이머 제로"), TEXT("인연 — 제로: 화면 밖 동료"), 7, {
        { TEXT("프로게이머 제로"), TEXT("팀게임 하면서도 난 늘 혼자였어. 다들 결과만 봤거든.") },
        { TEXT("프로게이머 제로"), TEXT("너넨 내가 져도 안 떠나더라. …그게, 좀 든든해.") },
    });

    // ── 인연 피날레 (랭크10 MAX) — 결속의 완성/약속 ──
    auto BondMax = [&](FName Id, const FString& NpcName, const FString& Title,
                       const TArray<FStoryLine>& Lines) -> FStoryBeat&
    {
        FStoryBeat& B = Bond(Id, NpcName, Title, 10, Lines);
        B.RewardGold = 200; // 피날레 보상 상향
        B.RewardItemId = TEXT("HiPotion"); B.RewardItemCount = 1; // 인연 MAX 기념 아이템
        return B;
    };
    BondMax(TEXT("bond_jihun_max"), TEXT("지훈"), TEXT("인연 MAX — 지훈: 영원한 러닝메이트"), {
        { TEXT("지훈"), TEXT("우리 졸업해도, 어른 돼도, 가끔은 같이 뛰자. 약속!") },
        { TEXT("나"),  TEXT("(지훈의 웃음이, 더는 가면처럼 보이지 않았다.)") },
    });
    BondMax(TEXT("bond_seoyeon_max"), TEXT("서연"), TEXT("인연 MAX — 서연: 마지막 페이지"), {
        { TEXT("서연"), TEXT("소설, 다 썼어요. 결말은… 주인공이 더는 숨지 않는 거예요.") },
        { TEXT("서연"), TEXT("당신 덕분에 쓸 수 있었어요. 이젠, 현실에서도 그럴게요.") },
    });
    BondMax(TEXT("bond_kanghyun_max"), TEXT("강현"), TEXT("인연 MAX — 강현: 등을 맡긴다"), {
        { TEXT("강현"), TEXT("이젠 안다. 진짜 강한 놈은, 혼자 안 싸워.") },
        { TEXT("강현"), TEXT("무슨 일이 와도 네 등은 내가 지킨다. 평생 친구 먹는 거다.") },
    });
    BondMax(TEXT("bond_minji_max"), TEXT("민지 쌤"), TEXT("인연 MAX — 민지 쌤: 나도 기댈게"), {
        { TEXT("민지 쌤"), TEXT("늘 누군가 치료해 주는 입장이었는데… 처음으로 기대고 싶은 사람이 생겼어.") },
        { TEXT("민지 쌤"), TEXT("고마워. 네 덕에 다시, 누군가를 돌볼 힘이 났어.") },
    });
    BondMax(TEXT("bond_detective_max"), TEXT("탐정 진"), TEXT("인연 MAX — 탐정 진: 영원한 파트너"), {
        { TEXT("탐정 진"), TEXT("내 인생 최고의 발견은, 사건이 아니라 너였어.") },
        { TEXT("탐정 진"), TEXT("어떤 미궁이 와도 함께라면 풀 수 있어. ……파트너.") },
    });
    BondMax(TEXT("bond_eunwoo_max"), TEXT("은우"), TEXT("인연 MAX — 은우: 진짜 연결"), {
        { TEXT("은우"), TEXT("세상이랑 단절돼 있다 생각했어. 근데 넌 내 방화벽을 다 뚫더라.") },
        { TEXT("은우"), TEXT("이건 영구 접속이야. 로그아웃 안 해. …절대로.") },
    });
    BondMax(TEXT("bond_dohyun_max"), TEXT("학생회장 도현"), TEXT("인연 MAX — 도현: 그냥 도현"), {
        { TEXT("학생회장 도현"), TEXT("회장도, 에이스도 아닌… 그냥 도현으로 있을 수 있는 곳이 생겼어.") },
        { TEXT("학생회장 도현"), TEXT("그게 너야. 평생 잊지 않을게.") },
    });
    BondMax(TEXT("bond_luna_max"), TEXT("연습생 루나"), TEXT("인연 MAX — 루나: 첫 무대 초대"), {
        { TEXT("연습생 루나"), TEXT("나 데뷔 확정됐어!! 첫 무대… 제일 앞자리, 네 자리야.") },
        { TEXT("연습생 루나"), TEXT("무대 위에서 너만 보고 노래할게. 약속!") },
    });
    BondMax(TEXT("bond_zero_max"), TEXT("프로게이머 제로"), TEXT("인연 MAX — 제로: 영원한 팀"), {
        { TEXT("프로게이머 제로"), TEXT("우승보다 값진 걸 얻었어. 지든 이기든 안 떠날 팀.") },
        { TEXT("프로게이머 제로"), TEXT("다음 시즌도, 그 다음도… 너랑 한 팀이다.") },
    });

    return Catalog;
}

bool UStoryManagerSubsystem::FindBeat(FName BeatId, FStoryBeat& OutBeat)
{
    if (BeatId.IsNone()) return false;
    for (const FStoryBeat& B : GetCatalog())
        if (B.BeatId == BeatId) { OutBeat = B; return true; }
    return false;
}

void UStoryManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Load();
}

TArray<FName> UStoryManagerSubsystem::GetCompletedBeatIds() const
{
    TArray<FName> Out;
    for (const FStoryBeat& B : GetCatalog())
        if (DoneBeats.Contains(B.BeatId))
            Out.Add(B.BeatId); // 카탈로그(서사) 순서 유지
    return Out;
}

int32 UStoryManagerSubsystem::GetCurrentChapter() const
{
    int32 Chapter = 1;
    for (const FStoryBeat& B : GetCatalog())
        if (DoneBeats.Contains(B.BeatId))
            Chapter = FMath::Max(Chapter, B.Chapter);
    return Chapter;
}

bool UStoryManagerSubsystem::GetNextAvailableBeat(int32 CurrentDay, FStoryBeat& OutBeat) const
{
    for (const FStoryBeat& B : GetCatalog())
    {
        if (DoneBeats.Contains(B.BeatId)) continue;                        // 이미 봄
        if (!B.PrereqBeatId.IsNone() && !DoneBeats.Contains(B.PrereqBeatId)) continue; // 선행 미완
        if (B.MinDay > 0 && CurrentDay < B.MinDay) continue;               // 날짜 미달
        if (!B.RequiredFlag.IsNone() && !Flags.Contains(B.RequiredFlag)) continue;     // 플래그 미충족
        {
            bool bAllFlags = true;                                          // 다중 플래그(트루엔딩 등) 전부 충족?
            for (FName F : B.RequiredAllFlags)
                if (!Flags.Contains(F)) { bAllFlags = false; break; }
            if (!bAllFlags) continue;
        }

        // 인연 랭크 게이트(인연 에피소드): 플레이어 RelationshipComponent에서 해당 NPC 랭크 조회
        if (!B.RequiredBondNPC.IsNone() && B.RequiredBondRank > 0)
        {
            int32 Rank = 0;
            if (UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
                if (APlayerController* PC = W->GetFirstPlayerController())
                    if (APawn* Pawn = PC->GetPawn())
                        if (URelationshipComponent* Rel = Pawn->FindComponentByClass<URelationshipComponent>())
                            Rank = Rel->GetRank(B.RequiredBondNPC);
            if (Rank < B.RequiredBondRank) continue;
        }

        OutBeat = B;
        return true;
    }
    return false;
}

void UStoryManagerSubsystem::SetFlag(FName Flag)
{
    if (Flag.IsNone() || Flags.Contains(Flag)) return;
    Flags.Add(Flag);
    Save();
}

void UStoryManagerSubsystem::CompleteBeat(FName BeatId)
{
    if (BeatId.IsNone() || DoneBeats.Contains(BeatId)) return;

    FStoryBeat Beat;
    if (!FindBeat(BeatId, Beat)) return;

    DoneBeats.Add(BeatId);
    if (!Beat.GrantsFlag.IsNone()) Flags.Add(Beat.GrantsFlag);

    // 보상(골드/아이템) → 플레이어(첫 컨트롤러 폰). 폰 1회 조회로 둘 다 처리.
    if (Beat.RewardGold > 0 || !Beat.RewardItemId.IsNone())
        if (UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
            if (APlayerController* PC = W->GetFirstPlayerController())
                if (APawn* Pawn = PC->GetPawn())
                {
                    if (Beat.RewardGold > 0)
                        if (UStatComponent* St = Pawn->FindComponentByClass<UStatComponent>())
                            St->AddGold(Beat.RewardGold);
                    if (!Beat.RewardItemId.IsNone())
                        if (UInventoryComponent* Inv = Pawn->FindComponentByClass<UInventoryComponent>())
                            Inv->AddItem(Beat.RewardItemId, FMath::Max(1, Beat.RewardItemCount));
                }

    Save();
    OnStoryBeatUnlocked.Broadcast(BeatId);
}

void UStoryManagerSubsystem::Save() const
{
    UStorySave* SaveObj = Cast<UStorySave>(
        UGameplayStatics::CreateSaveGameObject(UStorySave::StaticClass()));
    if (!SaveObj) return;
    SaveObj->DoneBeats = DoneBeats.Array();
    SaveObj->Flags = Flags.Array();
    UGameplayStatics::SaveGameToSlot(SaveObj, StorySlot(), 0);
}

void UStoryManagerSubsystem::Load()
{
    DoneBeats.Empty();
    Flags.Empty();
    if (!UGameplayStatics::DoesSaveGameExist(StorySlot(), 0)) return;
    if (UStorySave* SaveObj = Cast<UStorySave>(UGameplayStatics::LoadGameFromSlot(StorySlot(), 0)))
    {
        for (FName B : SaveObj->DoneBeats) DoneBeats.Add(B);
        for (FName F : SaveObj->Flags) Flags.Add(F);
    }
}
