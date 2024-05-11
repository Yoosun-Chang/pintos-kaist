# [WEEK07-11] 정글끝까지

```
 📢 “여기까지 잘 오셨습니다. OS프로젝트를 시작합니다.”
```

OS프로젝트는 PintOS의 코드를 직접 수정해가며 진행하는 프로젝트입니다.
PintOS는 2004년 스탠포드에서 만들어진 교육용 운영체제예요. 우리 프로젝트는 이를 기반으로 KAIST 권영진 교수님 주도 하에 만들어진 KAIST PintOS로 진행됩니다.

*****************************

#### 본격적인 탐험 - W07-11 정글끝까지

```
💡 팀 별로 프로젝트 1~3을 완수해갑니다.
```

* KAIST PintOS Assignment : https://casys-kaist.github.io/pintos-kaist/
* 내용은 어렵지만, 매우 상세하게 접근 방법을 기술하고 있습니다.


#### 작업환경 및 설정

```
💡 AWS EC2 Ubuntu 22.04 (x86_64)에서 진행합니다.
```

1) EC2를 다음과 같이 세팅합니다. (한 줄씩 입력)
```
  $ sudo apt update
  $ sudo apt install -y gcc make qemu-system-x86 python3
  ```
2) pintos repository 복제
아래와 같이 git 명령을 사용하여 GitHub에 있는 과제를 다운로드 하고 자기 팀의 git repository로 옮깁니다. 원본 repository에 있는 코드를 개선하기 위해 clone 혹은 fork하는 것이 아니고 팀의 repository로 복제(duplicate) 하는 것임을 명심하십시오.

참고: [GitHub: Duplicating a repository](https://docs.github.com/en/repositories/creating-and-managing-repositories/duplicating-a-repository)
```
  $ git clone --bare https://github.com/SWJungle/pintos-kaist.git
  $ cd pintos-kaist.git
  $ git push --mirror https://github.com/${교육생 or 팀ID}/pintos-kaist.git
  $ cd ..
  $ rm -rf pintos-kaist.git
  $ git clone https://github.com/${교육생ID}/pintos-kaist.git
```
팀 repository는 GitHub이 아니라 Bitbucket, Gitlab 등 다른 git repository여도 됩니다. 자신과 팀의 작업을 계속 기록하고 남겨두어서 작업방식의 변경, 개발 환경의 장애 발생 등에 대응할 수 있으면 됩니다.

3) PintOS가 제대로 동작되는지 확인 (Project 1의 경우)
```
  $ cd pintos-kaist
  $ source ./activate
  $ cd threads
  $ make check
  # 뭔가 한참 compile하고 test 프로그램이 돈 후에 다음 message가 나오면 정상
  20 of 27 tests failed.
```

***
1. PROJECT 1 - THREADS

    ✅ Alarm Clock  
    ✅ Priority Scheduling  
    ✅ Advanced Scheduler (Extra)  
    🚀 Result : `All 27 tests passed.`

******************
- [[Pintos-KAIST] Project 1 : Alarm Clock](https://velog.io/@ceusun0815/Pintos-KAIST-Project-1-Alarm-Clock) 
- [[Pintos-KAIST] Project 1 : Priority Scheduling](https://velog.io/@ceusun0815/Pintos-KAIST-Project-1-Priority-Scheduling)
- [[Pintos-KAIST] Project 1 : Priority Scheduling and Synchronization](https://velog.io/@ceusun0815/Pintos-KAIST-Project-1-Priority-Scheduling-and-Synchronization)
- [[Pintos-KAIST] Project 1 : Priority Donation](https://velog.io/@ceusun0815/Pintos-KAIST-Project-1-Priority-Donation)
- [[Pintos-KAIST] Project 1 : Multi-Level Feedback Queue Scheduler](https://velog.io/@ceusun0815/Pintos-KAIST-Project-1-Multi-Level-Feedback-Queue-Scheduler)
******************

Brand new pintos for Operating Systems and Lab (CS330), KAIST, by Youngjin Kwon.

The manual is available at https://casys-kaist.github.io/pintos-kaist/.

******************

Project 1
```
pass tests/threads/mlfqs/mlfqs-block
pass tests/threads/alarm-single
pass tests/threads/alarm-multiple
pass tests/threads/alarm-simultaneous
pass tests/threads/alarm-priority
pass tests/threads/alarm-zero
pass tests/threads/alarm-negative
pass tests/threads/priority-change
pass tests/threads/priority-donate-one
pass tests/threads/priority-donate-multiple
pass tests/threads/priority-donate-multiple2
pass tests/threads/priority-donate-nest
pass tests/threads/priority-donate-sema
pass tests/threads/priority-donate-lower
pass tests/threads/priority-fifo
pass tests/threads/priority-preempt
pass tests/threads/priority-sema
pass tests/threads/priority-condvar
pass tests/threads/priority-donate-chain
pass tests/threads/mlfqs/mlfqs-load-1
pass tests/threads/mlfqs/mlfqs-load-60
pass tests/threads/mlfqs/mlfqs-load-avg
pass tests/threads/mlfqs/mlfqs-recent-1
pass tests/threads/mlfqs/mlfqs-fair-2
pass tests/threads/mlfqs/mlfqs-fair-20
pass tests/threads/mlfqs/mlfqs-nice-2
pass tests/threads/mlfqs/mlfqs-nice-10
pass tests/threads/mlfqs/mlfqs-block
All 27 tests passed.
```
