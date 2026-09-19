# BUILD.md — Compilar o foo_sacd_dlna no Windows

Este documento explica, passo a passo, como preparar um PC Windows para **compilar, testar e diagnosticar** o `foo_sacd_dlna`.

> **Estado do projecto:** Alpha. O código precisa de ser compilado e testado em Windows com MSVC. A compatibilidade final com o T+A SDX 3100 HV só deve ser considerada validada depois de um teste com o firmware exacto do equipamento.

---

## 1. O que é necessário para compilar

### Software obrigatório

1. **Windows 64-bit**
2. **Visual Studio 2022** ou Build Tools 2022
3. Workload **Desktop development with C++**
4. **MSVC C++ Build Tools** para x64/x86
5. **Windows SDK**
6. **foobar2000 SDK 2025-03-07**
7. **foobar2000 64-bit** para testar o componente
8. **foo_input_sacd** para testar SACD ISO

O SDK usado neste projecto é o **2025-03-07**. A página oficial do foobar2000 indica que essa versão inclui projectos para Visual Studio 2019/2022. O changelog também indica que esta versão mantém C++17 em determinados projectos da SDK. 

- SDK oficial: https://www.foobar2000.org/SDK
- Changelog da SDK: https://www.foobar2000.org/changelog-sdk

### Software opcional, mas recomendado

- Git for Windows
- 7-Zip
- Python 3.x — para os scripts de diagnóstico DLNA
- Wireshark — para analisar SSDP/HTTP
- Windows Terminal

---

## 2. Hardware recomendado para a máquina de desenvolvimento

Para **compilar** o plugin, não é necessário hardware especial.

| Componente | Mínimo prático | Recomendado |
|---|---|---|
| CPU | 4 threads | 4–8+ núcleos físicos |
| RAM | 8 GB | 16 GB ou mais |
| Armazenamento | 20 GB livres | 40 GB+ num SSD/NVMe |
| GPU | integrada | integrada é suficiente |
| Rede | 100 Mbps | Gigabit Ethernet |

A Microsoft indica actualmente para Visual Studio 2022 um mínimo de 4 GB de RAM, processador x64/ARM64 e, para soluções profissionais típicas, 16 GB de RAM recomendados; as instalações típicas necessitam de 20–50 GB livres e a Microsoft recomenda SSD.

Para este projecto, **16 GB de RAM + SSD/NVMe** é uma configuração confortável, sobretudo quando se instala o Visual Studio, símbolos, SDKs e ferramentas adicionais.

---

## 3. Instalar o Visual Studio 2022

Descarrega o Visual Studio 2022 pela Microsoft:

https://visualstudio.microsoft.com/downloads/

Pode ser usada a edição **Community**, que é suficiente para este projecto.

No instalador selecciona:

```text
Workloads
└── Desktop development with C++
```

Este workload inclui os componentes essenciais de C++ para Windows, incluindo MSBuild e as ferramentas de build C++. A documentação Microsoft identifica o workload como `Microsoft.VisualStudio.Workload.VCTools`.

### Confirmar os componentes

Na instalação, confirma pelo menos:

```text
✓ MSVC C++ x64/x86 build tools
✓ MSBuild
✓ Windows SDK
✓ C++ core tools
```

O Windows SDK é instalado normalmente com o workload de desenvolvimento desktop C++.

---

## 4. Requisitos actuais do Visual Studio

Para a documentação de referência:

https://learn.microsoft.com/pt-pt/visualstudio/releases/2022/system-requirements

Actualmente, a Microsoft documenta suporte para versões 64-bit do Windows 11 e Windows Server suportado pela edição em causa. O Visual Studio 2022 requer .NET Framework 4.8 para funcionar e o instalador utiliza WebView2 quando necessário.

Para este projecto, a escolha recomendada é simples:

```text
Windows 11 x64
Visual Studio 2022
Desktop development with C++
```

---

## 5. Obter a SDK do foobar2000

A SDK **não deve ser incluída automaticamente neste repositório** sem verificar os termos de distribuição.

Descarrega-a directamente da página oficial:

https://www.foobar2000.org/SDK

A versão actualmente publicada na página oficial é:

```text
SDK 2025-03-07
```

A página oficial indica explicitamente project files para **Visual Studio 2019/2022**.

---

## 6. Estrutura de pastas recomendada

Uma estrutura simples é:

```text
C:\dev\
│
├── SDK-2025-03-07\
│   ├── foobar2000\
│   ├── helpers\
│   ├── pfc\
│   ├── shared\
│   └── ...
│
└── foo_sacd_dlna\
    ├── foo_sacd_dlna.sln
    ├── foo_sacd_dlna.vcxproj
    ├── dlna_server.cpp
    ├── sacd_decode.cpp
    └── ...
```

Mantém os dois projectos no mesmo nível para simplificar as referências relativas usadas pela solução.

---

## 7. Abrir o projecto

Abra:

```text
foo_sacd_dlna.sln
```

no Visual Studio 2022.

Selecciona:

```text
Configuration: Release
Platform: x64
```

Para depuração:

```text
Configuration: Debug
Platform: x64
```

Não uses `Win32`/x86 para este projecto.

---

## 8. Confirmar os caminhos da SDK

Se aparecer:

```text
cannot open include file ...
```

abre:

```text
Project → Properties
```

E confirma:

```text
C/C++ → Additional Include Directories
```

bem como:

```text
Linker → Additional Library Directories
```

As pastas da SDK, `pfc`, `shared` e restantes componentes têm de corresponder à SDK que extraíste.

Não assumes um caminho fixo como `C:\foobar2000-sdk`. Usa o caminho real da tua instalação.

---

## 9. Compilar

No Visual Studio:

```text
Build → Build Solution
```

ou:

```text
Ctrl + Shift + B
```

Configuração inicial recomendada:

```text
Release | x64
```

Uma compilação bem sucedida deve criar a DLL do componente no directório de output configurado pelo projecto.

---

## 10. Compilar a partir da linha de comandos

Abre:

```text
Developer Command Prompt for VS 2022
```

e executa:

```powershell
msbuild .\foo_sacd_dlna.sln /m /p:Configuration=Release /p:Platform=x64
```

Build limpo:

```powershell
msbuild .\foo_sacd_dlna.sln /t:Clean /p:Configuration=Release /p:Platform=x64
msbuild .\foo_sacd_dlna.sln /t:Build /m /p:Configuration=Release /p:Platform=x64
```

Se `msbuild` não for encontrado, estás provavelmente a usar uma consola normal em vez da Developer Command Prompt ou Developer PowerShell do Visual Studio.

---

## 11. Preparar um foobar2000 de teste

É altamente recomendado utilizar uma **instalação/perfil separado do foobar2000** para o desenvolvimento.

Não testes uma DLL Alpha directamente na tua instalação principal de música.

O ambiente de teste deve conter:

```text
foobar2000 x64
foo_input_sacd
foo_sacd_dlna
```

Depois reinicia o foobar2000.

---

## 12. Confirmar a dependência SACD

Abre:

```text
File → Preferences → Tools → SACD DLNA
```

Deve aparecer algo semelhante a:

```text
foo_input_sacd: INSTALLED
```

O `foo_input_sacd` é obrigatório para a parte **SACD ISO → DSD**.

O componente foi desenhado para não depender de funções privadas da DLL do SACD Decoder. O objectivo é usar as interfaces públicas do foobar2000 para pedir o fluxo DSD ao decoder instalado, permitindo actualizar o decoder independentemente.

---

## 13. Primeiro teste: DSF

Antes de testar SACD ISO, usa uma faixa `.dsf` que já saibas estar correcta.

Isto testa:

```text
Music Library
      ↓
foo_sacd_dlna
      ↓
UPnP/DLNA
      ↓
T+A SDX 3100 HV
```

Se o DSF não funcionar, não vale a pena começar por investigar o SACD Decoder.

---

## 14. Segundo teste: SACD ISO

Depois de DSF funcionar, testa:

```text
Album.iso
```

O caminho esperado é:

```text
SACD ISO
   ↓
foo_input_sacd
   ↓
DSD
   ↓
DSF cache
   ↓
HTTP/DLNA
   ↓
SDX 3100 HV
```

A ISO original não deve ser alterada.

---

## 15. Testar o servidor DLNA sem o SDX

O projecto inclui scripts em:

```text
tools\
```

O script de smoke test pode verificar a parte de MediaServer sem depender imediatamente do T+A:

```powershell
python .\tools\dlna_smoke_test.py 192.168.1.20 8192
```

Substitui `192.168.1.20` pelo IP do PC que executa o foobar2000.

O teste verifica elementos como:

```text
/device.xml
ContentDirectory::Browse
BrowseMetadata
ConnectionManager::GetProtocolInfo
/status
media resource
```

Para pedir também o recurso de áudio:

```powershell
python .\tools\dlna_smoke_test.py 192.168.1.20 8192 --get
```

---

## 16. Testar com o T+A SDX 3100 HV

A rede recomendada é:

```text
PC / foobar2000
      │
   Ethernet
      │
Gigabit Switch
      │
   Ethernet
      │
T+A SDX 3100 HV
```

O estado da interface deve distinguir:

```text
DLNA: BROADCASTING / ACTIVE
```

de:

```text
Audio stream: ACTIVE / TRANSMITTING
```

O primeiro indica que o servidor está disponível/discoverable.

O segundo indica que existe uma transferência HTTP de áudio activa.

Durante reprodução, procura algo como:

```text
T+A SDX: DETECTED / STREAMING
DSD: DSD256
TX: ~22–24 Mbit/s
```

---

## 17. Requisitos de rede

Débito aproximado do payload DSD estéreo:

```text
DSD64   ≈ 5.64 Mbit/s
DSD128  ≈ 11.29 Mbit/s
DSD256  ≈ 22.58 Mbit/s
```

TCP/IP, HTTP e UPnP acrescentam overhead.

Uma Ethernet de 100 Mbps é suficiente em teoria, mas para DSD256 a configuração recomendada é:

```text
Gigabit Ethernet
```

A razão é simples: o objectivo não é apenas ter largura de banda suficiente, mas também **ter margem** para outros dispositivos e congestionamento temporário.

---

## 18. Windows Firewall

Para o funcionamento local de DLNA, o componente usa normalmente:

```text
UDP 1900
```

para SSDP e:

```text
TCP 8192
```

para HTTP/media, por defeito.

Se mudares a porta nas Preferências, ajusta a regra do firewall.

Para o primeiro teste, permite o foobar2000 na rede **Private** do Windows.

Não exponhas este servidor DLNA directamente à Internet.

---

## 19. Testar HTTP Range

O renderer pode fazer pedidos parciais:

```http
Range: bytes=...
```

Isto é relevante para seek e determinados padrões de reprodução DLNA.

A resposta deve manter correctamente:

```text
206 Partial Content
Content-Range
Content-Length
Accept-Ranges: bytes
```

quando um pedido Range válido é recebido.

---

## 20. Diagnóstico com Wireshark

Para problemas de DLNA, Wireshark é extremamente útil.

Filtros úteis:

```text
ssdp
```

```text
http
```

```text
tcp.port == 8192
```

ou apenas o SDX:

```text
ip.addr == <IP-do-SDX>
```

Uma sessão típica deverá parecer-se com:

```text
SDX → SSDP M-SEARCH
PC  → SSDP response
SDX → GET /device.xml
SDX → SOAP Browse
SDX → SOAP BrowseMetadata
SDX → SOAP GetProtocolInfo
SDX → GET /media/<id>.dsf
```

O último pedido é o ponto em que começa a transmissão real do áudio.

---

## 21. Testar estabilidade DSD256

Configuração inicial:

```text
Stability Mode: ON
Pre-buffer: 15 s
```

Se a rede estiver muito ocupada:

```text
Pre-buffer: 20–30 s
```

Para redes muito instáveis:

```text
Pre-buffer: 30–60 s
```

Isto absorve variações temporárias. Não resolve uma ligação que fique permanentemente abaixo do débito necessário.

Para DSD256, 15 segundos correspondem a aproximadamente **42,3 MB** de payload DSD estéreo.

---

## 22. Testar cache SACD

Quando uma ISO é usada, o projecto pode criar uma cache DSF.

O comportamento pretendido é:

```text
Primeiro acesso
ISO → DSD → DSF cache

Acessos seguintes
DSF cache → DLNA
```

A cache deve ser invalidada quando o source ou os parâmetros relevantes mudam.

O objectivo é evitar que a conversão SACD seja repetida desnecessariamente e, ao mesmo tempo, desacoplar a descodificação da velocidade do cliente DLNA.

---

## 23. Teste de cancelamento/concurrency

Durante desenvolvimento, testa situações como:

1. iniciar uma faixa;
2. mudar rapidamente para outra;
3. cancelar enquanto uma ISO está a ser preparada;
4. abrir duas faixas/clients de forma concorrente;
5. desligar/reiniciar o renderer durante um streaming.

O componente deve cancelar tarefas antigas sem deixar ficheiros DSF incompletos a serem servidos como válidos.

---

## 24. Desenvolvimento no Visual Studio

Para depuração podes configurar o executável do foobar2000 como aplicação de arranque:

```text
Debug → foo_sacd_dlna Properties → Debugging
```

Exemplo:

```text
Executable:
C:\...\foobar2000.exe
```

O caminho deve apontar para a tua instalação de teste.

Áreas especialmente úteis para breakpoints:

```text
dlna_server.cpp
sacd_decode.cpp
dsf_writer.cpp
preferences.cpp
ui_element.cpp
```

---

## 25. Testes recomendados por ordem

Para reduzir o número de variáveis, testa por esta ordem:

```text
1. Compilar DLL
2. Carregar DLL no foobar2000
3. Preferences
4. foo_input_sacd detection
5. SSDP
6. device.xml
7. Browse
8. BrowseMetadata
9. DSF HTTP GET
10. HTTP Range
11. DSF no SDX
12. SACD ISO
13. DSF cache
14. DSD64
15. DSD128
16. DSD256
17. estabilidade/rede congestionada
18. gapless
19. artwork
20. reprodução longa
```

---

## 26. CI / GitHub Actions

O projecto inclui preparação para builds automáticos em Windows.

Um build automático deve produzir, no mínimo:

```text
BUILD: PASS
ARTIFACT: foo_sacd_dlna
HARDWARE VALIDATION: NOT RUN
```

Um build verde no GitHub Actions **não prova compatibilidade com o T+A**.

A validação física tem de ser feita com um SDX 3100 HV real e com o firmware exacto que estiver instalado.

---

## 27. Informações a guardar em cada release

Regista sempre:

```text
foo_sacd_dlna version
foobar2000 SDK version
Visual Studio version
MSVC toolset version
Windows SDK version
Target architecture
Configuration
Git commit
Windows version
T+A SDX firmware version
```

Exemplo:

```text
foo_sacd_dlna: 0.7.0-alpha3
foobar2000 SDK: 2025-03-07
Visual Studio: 2022
Configuration: Release
Platform: x64
```

---

## 28. Requisitos para utilizador final vs. programador

### Para desenvolver/compilar

```text
Windows x64
Visual Studio 2022 / Build Tools
Desktop development with C++
MSVC
Windows SDK
foobar2000 SDK
```

### Para executar o componente

```text
Windows x64
foobar2000 x64
foo_sacd_dlna
foo_input_sacd (necessário para SACD ISO)
rede local
```

O utilizador final **não precisa de Visual Studio nem da SDK** para utilizar uma versão já compilada do componente.

---

## 29. Checklist antes de publicar uma release

```text
[ ] Release | x64 compila sem erros
[ ] Sem DLL Debug no pacote
[ ] foobar2000 carrega o componente
[ ] Preferences funciona
[ ] Help funciona
[ ] foo_input_sacd é detectado
[ ] SSDP funciona
[ ] Browse funciona
[ ] BrowseMetadata funciona
[ ] DSF abre no renderer
[ ] HTTP Range funciona
[ ] SACD ISO funciona
[ ] Cache funciona
[ ] DSD64 testado
[ ] DSD128 testado
[ ] DSD256 testado
[ ] Artwork testado
[ ] Cancelamento testado
[ ] Concorrência testada
[ ] Firewall documentado
[ ] Logs verificados
[ ] Firmware T+A registado
[ ] Hardware T+A testado
```

---

## 30. Referências oficiais

### foobar2000

SDK:
https://www.foobar2000.org/SDK

SDK changelog:
https://www.foobar2000.org/changelog-sdk

### Microsoft

Visual Studio 2022 — requisitos:
https://learn.microsoft.com/pt-pt/visualstudio/releases/2022/system-requirements

Desktop development with C++ / workload:
https://learn.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-build-tools

MSVC Build Tools:
https://learn.microsoft.com/en-us/cpp/overview/acquire-msvc

Windows C++ development:
https://learn.microsoft.com/en-us/cpp/windows/overview-of-windows-programming-in-cpp


## foobar2000 SDK / MSVC toolset used by this project

The supplied foobar2000 SDK project tree is configured for the **v142** C++ toolset. The `foo_sacd_dlna.vcxproj` included in this repository is therefore configured explicitly for:

```text
Platform: x64
Toolset: v142

Debug   -> v142
Release -> v142
```

### Required Visual Studio components for v142

Install these through **Visual Studio Installer → Modify → Individual components**:

- **MSVC v142 - VS 2019 C++ x64/x86 build tools (v14.29)**
- **C++ ATL for latest v142 build tools (x86 & x64)** / the ATL component for v142
- A Windows SDK
- MSBuild / C++ build tools (provided by the Desktop development with C++ workload)

The important point is that installing only the newer v143 ATL does not solve a project that is explicitly built with v142. The SDK/project and the component should use the same intended toolset.

After changing/installing the toolset:

1. Close Visual Studio.
2. Reopen `foo_sacd_dlna.sln`.
3. Confirm **Debug | x64** or **Release | x64**.
4. Run **Build → Clean Solution**.
5. Run **Build → Rebuild Solution**.

If `atlbase.h` is still missing, verify that the v142 ATL component is installed and that Visual Studio can see the v142 toolchain.
