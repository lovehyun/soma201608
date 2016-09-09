// http://hackerschool.org/~research/study/SS_1.htm

// 일반적인 헤더 파일 포함
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Socket 관련 헤더 파일 포함
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// tcp/ip 관련 헤더 파일 포함
#include <linux/ip.h>
#include <linux/tcp.h>

int main() {
	// 이제부터 우리가 만들 Packet이 저장되는 변수.
	// TCP 헤더와 IP 헤더가 각각 20바이트 임으로 총 40바이트가 되었습니다.
	unsigned char packet[40];

	// 로우 소켓 디스크립터.
	int raw_socket;

	// 참을 나타내는 값인 1이 저장되는 변수. 용도는 뒤에 설명~!
	int on = 1;

	// IP 헤더 구조체 변수
	struct iphdr *iphdr;

	// TCP 헤더 구조체 변수.
	struct tcphdr *tcphdr;

	// sendto() 함수에서 사용할 받는 이의 주소 값.
	struct sockaddr_in address;

	// 이제 변수 선언은 끝났습니다. 그럼 지금부터 로우 소켓을 만들어 볼까요?

	// 두 번째와 세 번째 인자에 주목! 평소에는 SOCK_STREAM와 IPPROTO_TCP를
	// 사용했었죠? 하지만 로우 소켓을 만들 때는 아래와 같은 값을 넣어줘야
	// 합니다. 그리고 최대한 소스를 간결하게 하기 위해 에러 처리는 생략하도록
	// 하겠습니다.
	raw_socket = socket( AF_INET, SOCK_RAW, IPPROTO_RAW);

	// 함수 명 그대로 소켓의 옵션을 세팅하겠다~ 라는 뜻의 함수입니다.
	// 두 번째 인자는 프로토콜의 레벨, 세, 네 번째 인자는 각각 옵션 항목과
	// 값을 의미하고, 마지막 인자는 그 값의 크기를 의미합니다.
	// 여기서 함수 원형을 보면 네 번째 인자는 주소 값을 요구하므로, 1이라는
	// 값이 저장된 on 이라는 변수를 만든 후 그 변수의 주소 값을 넣어
	// 주었습니다. 우리가 설정한 값은 "이제부터 패킷의 헤더를 직접 만들겠다!"
	// 를 의미합니다.

	setsockopt(raw_socket, IPPROTO_IP, IP_HDRINCL, (char *) &on, sizeof(on));
	// 함수 원형 ->
	// int setsockopt( int s, int level, int name, const void *val, int len )

	// 자~! 그럼 이제부터 TCP 헤더를 직접 만들어 볼까요? 우리가 보통
	// TCP/IP라고 말하지만 실제 Packet에서는 IP 헤더가 앞쪽에 위치 한답니다.
	// 따라서 다음과 같이 앞 20바이트에 IP 헤더가 들어갈 20바이트의
	// 자리를 남겨두어야 합니다.
	// 이 부분이 이해 안 가시는 분은 포인터 공부를 먼저 하셔야 할겁니다.
	tcphdr = (struct tcphdr *) (packet + 20);

	// 모든 값을 0으로 초기화.
	memset((char *) tcphdr, 0, 20);

	// 발신자의 발신 포트, 마음대로 지정~!
	tcphdr->source = htons(777);

	// 수신자의 포트 지정.
	tcphdr->dest = htons(12345);

	// Sequence Number 지정. 보통 10자리 미만의 숫자 값.
	tcphdr->seq = htonl(92929292);

	// Acknowledgement Number의 값. 마찬가지.
	tcphdr->ack_seq = htonl(12121212);

	// offset의 값 지정.
	tcphdr->doff = 5;

	// SYN flag 설정
	tcphdr->syn = 1;

	// 윈도우 사이즈.
	tcphdr->window = htons(512);

	// 메시지가 무사히 도착했는지 나타내는 값. 일단 임의의 값인 1로 설정.
	// 중요한 변수라는 걸 꼭 기억해 두세요.
	tcphdr->check = 1;

	// * 자~! 이제 TCP 헤더 제작이 끝났습니다. 간단하죠? ^^
	// 여기서 잠깐, 쉬운 이해를 위해 우리가 만든 TCP 헤더를 도식화 해봅시다

	//	[ 그림으로 표현된 TCP 헤더 ] - [ 우리가 값을 채워넣은 모습 ]

	// 직접 값을 지정해 주지 않은 부분은 처음에 초기화했던 대로 0 입니다.
	// 그럼 이번엔 IP 헤더를 만들어 보도록 합시다.

	// 우리가 설정하는 값이 직접 packet 변수에 저장되도록 연결해 줍니다.
	iphdr = (struct iphdr *)packet;

	// 모든 값을 0으로 초기화.
	memset((char *) iphdr, 0, 20);

	// IP Version의 값은 4로 설정.
	iphdr->version = 4;

	// IP Header Length의 값을 5로 설정.
	iphdr->ihl = 5;

	// protocol을 TCP로 설정.
	iphdr->protocol = IPPROTO_TCP;

	// 패킷의 총 길이는 40으로 설정.
	iphdr->tot_len = 40;

	// 각각의 패킷을 구분할 때 쓰이는 ID, 임의의 값으로 설정.
	iphdr->id = htons(777);

	// 기본 Time To Live의 값.
	iphdr->ttl = 60;

	// 체크섬 값.. 중요한 것이라고 말했죠? 기억하고 계세요.
	iphdr->check = 1;

	// 발신자의 IP 설정. 내 마음대로~!
	iphdr->saddr = inet_addr("111.111.111.111");

	// 수신자의 IP 설정. 이건 실제 패킷을 받게 될 IP를 적어줘야겠죠?
	// 이 강좌에서는 외부에 있는 리눅스 서버(211.189.88.58)에서 제 윈도우
	// 컴퓨터(218.149.4.60)로 패킷을 보내는 예로 설명할 것입니다.
	iphdr->daddr = inet_addr("218.149.4.60");

	// * 드디어 IP 헤더 제작도 끝났습니다.~
	// 역시 눈으로 한번 확인해 보도록 합시다.

	// [ 그림으로 표현된 IP 헤더 ] - [ 우리가 값을 채워넣은 모습 ]

	// 자~! 그럼 이제 지금까지 제작한 패킷을 날려 볼까요?

	// 일단 패킷을 받게 될 곳의 주소 정보를 지정합니다.
	// 참고로 아까 지정한 포트와 주소는 헤더에 저장되는 값이고,
	// 이번에 적어주는 것은 sendto() 함수에서 사용하는 것입니다.
	address.sin_family = AF_INET;
	address.sin_port = htons(12345);
	address.sin_addr.s_addr = inet_addr("218.149.4.60");

	// 패킷 발사!!
	sendto(raw_socket, &packet, sizeof(packet), 0x0,
			(struct sockaddr *) &address, sizeof(address));

}

