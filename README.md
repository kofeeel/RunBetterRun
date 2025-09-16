<img src="https://github.com/leebo155/RunBetterRun/blob/main/screenshots/main.png">

## 개요
* 타계는 [다른 세계에 가는 방법](https://namu.wiki/w/%EB%8B%A4%EB%A5%B8%20%EC%84%B8%EA%B3%84%EC%97%90%20%EA%B0%80%EB%8A%94%20%EB%B0%A9%EB%B2%95#toc)이란 괴담을 게임으로 표현한 프로젝트입니다.
* [Dark Deception](https://store.steampowered.com/app/332950/Dark_Deception/)을 벤치마킹하여 제작하였습니다. 주인공은 미로와 같은 공간을 돌아다니며 추격해오는 몬스터들을 피해 아이템들을 모아 스테이지를 탈출해야 합니다.
* 주로 [GitHub Project](https://github.com/orgs/PotenUpRunBetterRun/projects/1/views/2)를 통해 협업 하였습니다.
* 개발기간은 2025.4.12 ~ 2025.4.25, 약 2주 소요되었습니다.
    

### 플레이
<a href="https://kofeeel.itch.io/transmundus">
  <img src="https://static-00.iconduck.com/assets.00/itch-io-icon-512x512-wwio9bi8.png" alt="itch.io" width="100"/>
</a>

### 사용 기술
* C++
* SDL 2.32.4
* SDL Mixer 2.8.1
      
### 제작자
|<img src="https://github.com/leebo155.png" width=240>|<img src="https://github.com/shng6815.png" width="240">|<img src="https://github.com/kofeeel.png" width=240>|<img src="https://github.com/Baekbanjang.png" width=240>|
|:--:|:--:|:--:|:--:|
|[Bohyeong Lee](https://github.com/leebo155)|[SEO HUIYEONG](https://github.com/shng6815)|[Hasimu](https://github.com/kofeeel)|[Baekbanjang](https://github.com/Baekbanjang)|
|PD, 게임아트|UI/UX, 사운드, 카메라|에디터, 데이터 관리|Scene, 추적 알고리즘|  

### 담당 업무
* **커스텀 TileMapEditor** - 인게임 에디터 제작
* **바이너리 파일 시그니처 기반 데이터 직렬화** - 파일 무결성 검증 및 효율적인 리소스 관리 시스템 개발
* **맵 매니저 구현** - 런타임에서 맵 정보를 관리하는 매니저
* **데이터 매니저 구현** - 모든 인게임 데이터의 저장/로드 기능을 담당하는 매니저
* **레벨 디자인**<br>

### 레이캐스팅 관련 학습
<https://kofeeel.tistory.com/53>

## 구현한 부분

### 에디터 개발

#### 레퍼런스 에디터 
<a href="https://github.com/kofeeel/RunBetterRun/tree/main/RunBetterRun/Image/image&20(1)">
  <img src="RunBetterRun/Image/image%20(1).png" alt="레퍼런스 에디터" width="600">
</a>

#### 구현한 에디터
<a href="https://github.com/leebo155/RunBetterRun/blob/main/Image/editor.png">
  <img src="RunBetterRun/Image/image.png" alt="에디터" width="900">
</a>

#### 개요
* 맵에디터 정리글 <https://kofeeel.tistory.com/58>

#### 에디터 기능 
* 오브젝트(엔티티) 타입 별로 모드를 구분해서 다른 타입의 오브젝트는 삭제할 수 없도록 함
* 숏컷 기능: 저장(s), 로드(l), 다른 이름으로 저장(a), 맵 초기화(c), 타일 중앙배치 토글(i), 드래그(토글 d,우클릭 배치, 좌클릭 삭제), 확대/축소(마우스 휠), 스크롤(마우스 휠클릭)
* 샘플영역에서 배치할 타일, 오브젝트를 볼 수 있음
* 맵 저장시 가장자리는 항상 벽을 생성하게 함: Ray의 거리가 무한대가 되지 않고 충돌할 수 있게 예외처리
* 플레이어 시작위치의 타일은 항상 바닥 타입으로 설정하게 함 : Ray의 거리가 0이 되지않게 예외처리

&nbsp;

### 바이너리 파일 시스템 구현

#### 개요
* 파일시그니처 정리글 <https://kofeeel.tistory.com/59>
* 파일 시그니처란?
 파일 시그니처는 파일의 시작 부분에 위치한 특정 바이트 시퀀스로, 해당 파일의 유형이나 형식을 식별하는 데 사용됩니다.
 일반적으로 "매직 넘버(Magic Number)"라고도 불리며, 파일을 열 때 어떤 프로그램이나 처리 방식이 필요한지 결정하는 중요한 정보를 제공합니다.

#### 프로젝트에서의 파일 시그니처 활용
 이번 프로젝트에서는 맵 데이터를 효율적으로 관리하기 위해 바이너리 파일 형식을 사용하면서, 파일 시그니처 개념을 적용했습니다. 게임에서 사용되는 데이터 파일의 종류에 따라 고유한 시그니처를 정의했습니다:

#### 주요 특징
* 파일 시그니처: "M,P,D,T"(Map Data) 시그니처로 파일 유효성 검증
* 구조화된 헤더: 맵 정보, 버전, 에셋 경로 등을 포함한 파일 헤더
* 데이터 무결성: 로드 시 시그니처 및 구조 검증

### 프로젝트 회고록
https://kofeeel.tistory.com/62


### 영상
[![타계시연1](http://img.youtube.com/vi/owF7KMpwQAQ/0.jpg)](https://youtu.be/owF7KMpwQAQ?t=0s)
[![타계시연2](http://img.youtube.com/vi/YLMpeg3B13g/0.jpg)](https://youtu.be/YLMpeg3B13g?t=0s)<br>  


### 스크린샷
<img src="https://github.com/leebo155/RunBetterRun/blob/main/screenshots/1.jpg">
<img src="https://github.com/leebo155/RunBetterRun/blob/main/screenshots/2.jpg">
<img src="https://github.com/leebo155/RunBetterRun/blob/main/screenshots/3.jpg">
<img src="https://github.com/leebo155/RunBetterRun/blob/main/screenshots/4.jpg">
<img src="https://github.com/leebo155/RunBetterRun/blob/main/screenshots/5.jpg">
<img src="https://github.com/leebo155/RunBetterRun/blob/main/screenshots/6.jpg">
<img src="https://github.com/leebo155/RunBetterRun/blob/main/screenshots/7.jpg">
<img src="https://github.com/leebo155/RunBetterRun/blob/main/screenshots/8.jpg">



