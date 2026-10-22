# Missing Games (3DS homebrew)

설치된 3DS 게임(SD/NAND)을 스캔해서, 국가별(한국/북미/유럽/일본/대만)로 아직 설치 안 한 게임을 보여줍니다.

## 빌드 (GitHub)
1. 이 폴더 내용을 저장소에 그대로 업로드 (.github 폴더 포함)
2. Actions 탭 -> "Build 3dsx" 완료 후 Artifacts에서 `3dsx`(앱)와 `db`(게임 목록) 다운로드
   (db.txt는 저장소의 sdcard_example/db.txt 에도 미리 들어 있습니다)

## 설치
SD:/3ds/MissingGames/ 에 `MissingGames.3dsx` 와 `db.txt` 를 넣으세요.
(3ds 폴더 안에 넣으면 홈브류 런처에서 바로 실행됩니다)

## 조작
- 하단 터치 또는 L/R : 국가 전환
- 상/하 : 스크롤, 좌/우 : 페이지
- Y : 같은 게임의 다른 지역판을 갖고 있으면 숨김 on/off
- A : missing.txt 저장(국가별 구분)
- START : 종료

## db.txt 갱신
`python3 tools/make_db.py db.txt` (데이터 출처: github.com/hax0kartik/3dsdb)
