# 100k_IO_EchoServer
Cpp Network, IOCP
Cs Network, async/await

## 초당 10만회의 패킷처리를 수행할 수 있는 에코서버를 만들어보자.
[기존 진행하던 프로젝트](https://github.com/SuhYC/RPGServer)는 C++환경과 C#환경 사이에서의 원활한 네트워크 송수신을 위해 링버퍼를 도입하고, <br/>
패킷 분할을 처리하기 위해 ```[size]```와 같은 헤더를 부착하여 송신하고, 수신 측에서 해당 헤더를 제거하는 작업을 수행했다. <br/>
이 과정에서 ```[size]```와 같은 문자열을 부착하고 처리하는 과정은 상당히 비효율적이다. <br/>
```std::string::find```함수는 결국 문자열의 앞부터 순회하여 해당하는 문자가 발견될 때까지 하나씩 검사하기 때문에 O(N)의 복잡도를 갖는다. <br/>
헤더의 길이는 logN이라고 볼 수 있으니 O(logN)이라고 볼 수도 있지만, 이보다 더 간단하게 헤더처리를 할 수 있다. <br/>

## 송신문자열의 상위 4바이트를 항상 헤더로 고정, unsigned int 데이터를 활용해 길이를 표현.
방법은 간단하다. 송신할 페이로드의 크기를 ```unsigned int``` 형으로 표현하고 <br/>
해당 데이터를 그대로 메모리 복사하여 송신문자열의 상위4바이트로 두고, <br/>
페이로드를 그 뒤에 복사하여 붙이고 전송하는 것. <br/>

아래 그림이 송신큐에 담기 위해 헤더를 부착하는 부분이다. <br/>
![SendQueue.Push](https://github.com/SuhYC/100k_IO_EchoServer/blob/main/push.png)

수신 측에서도 최대한 메모리 복사를 이용해 처리한다. <br/>
상위 4바이트를 ```unsigned int```형 데이터에 메모리 복사하여 페이로드의 크기를 알아내고, <br/>
문자열의 크기가 충분하다면 헤더와 페이로드에 해당하는 부분을 제거하여 <br/>
페이로드는 처리함수로 보내고 남은 문자열은 앞으로 당겨주는 식으로 처리할 수 있다. <br/>

아래 그림이 수신큐에서 처리할 페이로드를 가져오는 부분이다. <br/>
![RecvQueue.Pop](https://github.com/SuhYC/100k_IO_EchoServer/blob/main/HandleHeader.png)

## 측정 방법
클라이언트의 송신큐에 미리 250개 가량의 메시지를 담아두고 시작한다.<br/>
클라이언트의 송수신 스레드는 각 1개씩 사용하며, 서버는 각각의 메시지의 헤더를 제거한 뒤 바로 헤더를 다시 부착하여 클라이언트로 재송신한다. <br/>
클라이언트의 수신스레드가 문자열을 수신하면 헤더를 제거하여 페이로드를 획득할 때마다 송수신 횟수를 1회 증가시킨다. 이후 페이로드는 다시 헤더를 부착하여 서버로 전송한다.<br/>
100만회 송수신이 완료되면 소요된 시간을 출력한다.

## 결과
![결과1](https://github.com/SuhYC/100k_IO_EchoServer/blob/main/1.png) <br/>
![결과2](https://github.com/SuhYC/100k_IO_EchoServer/blob/main/2.png) <br/>
![결과3](https://github.com/SuhYC/100k_IO_EchoServer/blob/main/3.png) <br/>
![결과4](https://github.com/SuhYC/100k_IO_EchoServer/blob/main/4.png) <br/>
![결과5](https://github.com/SuhYC/100k_IO_EchoServer/blob/main/5.png) <br/>

5회 실행 결과는 100만 패킷 기준으로 10226 밀리초, 10375 밀리초, 9800 밀리초, 9710 밀리초, 10451 밀리초가 측정되었다. <br/>
10만회의 송수신에는 평균 1.01124 초가 소요되었다. <br/>
5회의 실행 중 2회의 실행에서는 초당 10만 패킷 이상의 송수신을 하는데 성공하였으며 <br/>
초당 10만회 이상의 송수신을 매 시도마다 하지는 못하더라도 10만회에 근접한 송수신을 성공하는 모습을 보였다.

## C++과 C# 사이에서도 가능할까?
C#에서도 어느정도의 메모리복사연산이 가능하며(```Buffer.BlockCopy```), 이를 통해 링버퍼와 헤더처리를 적용할 수 있다. <br/>
또한 ```async/await```를 통한 비동기 로직이 가능하기 때문에 비동기 네트워크를 이용하여 더욱 더 효율적인 IO가 가능하다. <br/>

4바이트 헤더의 처리를 위해 메모리연산을 사용하는 과정에서 비트연산을 통해 처리해보고자 하는 시도가 있었는데, <br/>
여기서 알게된 사실은 ```uint``` 데이터(혹은 C++의 ```unsigned int```)는 리틀엔디안으로 표현된다는 사실이다. <br/>
쉽게 말하면 0x00000005를 저장한다고 가정하면 byte[]배열에 byte[0] = 0x05, byte[1] = 0x00, byte[2] = 0x00, byte[3] = 0x00으로 저장된다는 것. <br/>
모든 아키텍처가 리틀엔디안을 사용하는 것은 아니지만 대부분의 아키텍처에서는 이렇게 적용하는 것이 더 효율적이라고 한다.

## 초기 시도
초기 시도에서는 초당 3500회(심지어 Unity에서 시도했을 때에는 초당 1000회 수준이었다.)의 패킷처리밖에 불가능했는데,<br/>
이 이유를 찾아보니 async/await를 온전히 다루지 못했던 것에 있었다. <br/>

다음 사진 3장이 개선 전의 코드인데, <br/>
![recv](https://github.com/SuhYC/100k_IO_EchoServer/blob/main/CppToCs_Result/recv.png) <br/>
![sendmsg](https://github.com/SuhYC/100k_IO_EchoServer/blob/main/CppToCs_Result/sendmsg.png) <br/>
![sendIO](https://github.com/SuhYC/100k_IO_EchoServer/blob/main/CppToCs_Result/sendio.png) <br/>

```RecvMsg```에서 수신이 완료되면, 헤더처리 클래스인 ResHandler에서 헤더를 처리한 후, <br/>
```SendMsg```함수로 넘겨 송신 큐에 담고, <br/>
바로 이어서 ```SendIO```함수에서 송신까지 마친 후에 다시 ```RecvMsg```의 실행흐름으로 돌아와 반복문을 돌게 된다. <br/>

즉, 수신이 완료되어야 처리를 할 수 있고, 처리가 완료되어야 송신을 할 수 있는 구조인 셈이다. <br/>

## 개선
이를 다음과 같이 개선하였다. <br/>
![NewSendMsg](https://github.com/SuhYC/100k_IO_EchoServer/blob/main/CppToCs_Result/NewSendMsg.png) <br/>
![NewSendIO](https://github.com/SuhYC/100k_IO_EchoServer/blob/main/CppToCs_Result/NewSendIO.png) <br/>

```SendMsg```는 이제 송신큐에 데이터를 담기만 하고 바로 ```RecvMsg```의 실행흐름으로 돌아와 반복문을 돈다. <br/>
```SendIO```는 별도의 실행흐름으로 송신큐를 확인하고 전송할 메시지가 있으면 송신한다. 없다면 ```Task.Delay(0);```를 통해 다른 실행흐름을 실행하도록 한다. <br/>

즉, 수신 흐름에서 헤더 처리한 데이터를 송신큐에 담는 작업과, 송신큐에서 데이터를 가져와 송신하는 작업을 분리하여 비동기적으로 실행할 수 있도록 개선하였다. <br/>

## Echo Test Between Cs and Cpp 결과
![결과1](https://github.com/SuhYC/100k_IO_EchoServer/blob/main/CppToCs_Result/1.png) <br/>
![결과2](https://github.com/SuhYC/100k_IO_EchoServer/blob/main/CppToCs_Result/2.png) <br/>
![결과3](https://github.com/SuhYC/100k_IO_EchoServer/blob/main/CppToCs_Result/3.png) <br/>
![결과4](https://github.com/SuhYC/100k_IO_EchoServer/blob/main/CppToCs_Result/4.png) <br/>
![결과5](https://github.com/SuhYC/100k_IO_EchoServer/blob/main/CppToCs_Result/5.png) <br/>

5회 실행 결과는 10만 패킷 기준으로 1426ms, 1340ms, 1414ms, 1192ms, 1398ms가 소요되었으며, <br/>
평균적으로 1354ms가 소요되었다. <br/>
즉, 초당 평균 73855개의 패킷을 처리할 수 있었다.
