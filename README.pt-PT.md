# foo_sacd_dlna

Componente de servidor UPnP/DLNA de DSD nativo para [foobar2000](https://www.foobar2000.org/).

> **Estado: Alpha / desenvolvimento**
>
> Este repositório contém o código-fonte actual do `foo_sacd_dlna` v0.7 Alpha 3. Ainda não é uma versão final nem uma componente oficial do foobar2000.

## Objectivo

O `foo_sacd_dlna` foi concebido para disponibilizar a parte DSD da Music Library do foobar2000 através de UPnP/DLNA para leitores de áudio de rede compatíveis.

O alvo inicial é o **T+A SDX 3100 HV**, mantendo a seguinte política:

- apenas DSD no caminho de rede;
- nenhuma conversão DSD → PCM pelo `foo_sacd_dlna`;
- nenhum DoP enviado para o leitor de rede;
- entrega de `DSF` / `DFF` nativos quando suportados pelo equipamento;
- SACD ISO depende do **Super Audio CD Decoder (`foo_input_sacd`)** instalado separadamente;
- utilização da interface pública do decoder do foobar2000, sem carregar APIs privadas da DLL do `foo_input_sacd`.

## Funcionalidades da V0.7 Alpha 3

- Página dedicada **Preferências → Tools → SACD DLNA**.
- Menu próprio **Tools → SACD DLNA**.
- Elemento opcional **SACD DLNA Status**.
- Estado visível `BROADCASTING / ACTIVE`.
- Verificação da instalação do `foo_input_sacd`.
- Detecção da versão do SACD Decoder quando disponível.
- Nome do servidor configurável.
- Porta HTTP configurável.
- Activação/desactivação do broadcasting DLNA.
- Partilha da Music Library DSD.
- Estrutura **Artista → Álbum → Faixa**.
- Album art através do sistema de artwork do foobar2000.
- SSDP / UPnP MediaServer / ContentDirectory / ConnectionManager.
- HTTP `Range`.
- DSF/DFF directamente e SACD ISO com cache DSF temporário.
- DSD64 / DSD128 / DSD256.
- Ajuda integrada.

## Fluxo de SACD ISO

```text
SACD ISO
   ↓
foo_input_sacd
   ↓
API pública input_decoder do foobar2000
   ↓
DoP interno
   ↓
extracção dos bits DSD
   ↓
DSF
   ↓
UPnP / DLNA / HTTP
   ↓
T+A SDX 3100 HV
```

O DoP acima é apenas um mecanismo interno entre o decoder e o componente. Não é enviado para a rede.


### Fluxo DLNA real

A versão actual implementa o fluxo principal de um MediaServer UPnP/DLNA real:

```text
T+A SDX 3100 HV
      │
      ├── SSDP discovery
      ├── Device Description
      ├── ContentDirectory Browse/BrowseMetadata
      ├── ConnectionManager GetProtocolInfo
      │
      └── HTTP GET /media/<id>.dsf
                     │
                     └── DSF / SACD ISO → foo_input_sacd → cache DSF
```

O estado do sistema mostra separadamente `BROADCASTING` e `TRANSMITTING`, incluindo TX, DSD, cliente activo e detecção do T+A.

## Requisitos de Hardware e Rede

### Hardware mínimo

O `foo_sacd_dlna` é um componente do foobar2000 e **não necessita de uma placa gráfica dedicada**.

Para uma instalação Windows prática:

| Componente | Mínimo | Recomendado |
|---|---|---|
| CPU | 2 núcleos físicos / 4 threads | 4+ núcleos físicos |
| RAM | 4 GB | 8 GB ou mais |
| Disco do sistema | SSD preferível | SSD |
| Armazenamento da música | HDD/SSD/NAS | SSD para a cache SACD/DSF |
| Rede | Ethernet 100 Mbps | Ethernet 1 Gbps |
| GPU | Não necessária | A gráfica integrada é suficiente |
| SO | Windows 7 ou mais recente | Windows actual 64-bit |
| foobar2000 | 64-bit recomendado | Versão actual 64-bit |
| `foo_input_sacd` | **Obrigatório para SACD ISO** | Versão actual |

Os requisitos oficiais actuais do foobar2000 para Windows indicam Windows 7 ou mais recente. O componente não utiliza aceleração por GPU.

### Largura de banda necessária para DSD

O `foo_sacd_dlna` mantém o DSD nativo e não o comprime.

Débitos aproximados para DSD estéreo:

| Formato | Relógio DSD | Débito aproximado |
|---|---:|---:|
| DSD64 | 2,8224 MHz | 5,64 Mbit/s |
| DSD128 | 5,6448 MHz | 11,29 Mbit/s |
| DSD256 | 11,2896 MHz | 22,58 Mbit/s |

Estes valores correspondem apenas ao payload de áudio DSD. TCP/IP, HTTP e DLNA/UPnP acrescentam algum tráfego.

Uma rede Ethernet de 100 Mbps é, portanto, **teoricamente suficiente para DSD256**, mas **Ethernet de 1 Gbps é fortemente recomendada** para obter estabilidade numa rede doméstica com outro tráfego.

### Topologia recomendada

Para o streaming DSD256 mais estável:

```text
                 Ethernet 1 Gbps
                       │
                       ▼
              ┌────────────────┐
              │ Switch Gigabit │
              └───────┬────────┘
                      │
              ┌───────┴────────┐
              │                │
              ▼                ▼
       PC Windows /       T+A SDX 3100 HV
       foobar2000
```

O PC e o SDX devem, de preferência, estar ligados por Ethernet ao mesmo switch/router.

Wi-Fi 5 GHz pode fornecer largura de banda suficiente para DSD256, mas Ethernet é preferível porque oferece latência mais previsível e é menos afectada por interferências rádio e congestionamento. O SDX 3100 HV dispõe de Ethernet 10/100/1000 Base-T e Wi-Fi.

### Rede por velocidade DSD

| Rede | DSD64 | DSD128 | DSD256 |
|---|---|---|---|
| Ethernet 100 Mbps | ✓ | ✓ | ✓* |
| Ethernet 1 Gbps | ✓ | ✓ | **Recomendada** |
| Wi-Fi 2,4 GHz | ✓* | Possível* | Não recomendada |
| Wi-Fi 5 GHz | ✓ | ✓ | Possível* |

`*` A estabilidade real depende do restante tráfego de rede, qualidade do sinal, retransmissões, desempenho do switch/router e desempenho do armazenamento.

### Stability Mode

Para redes congestionadas, activar o **Stability Mode**.

A configuração predefinida utiliza uma estratégia de read-ahead/cache de 15 segundos. Isto separa a descodificação do SACD da transmissão pela rede, para que pequenas quebras temporárias de velocidade não interrompam imediatamente a reprodução.

Em DSD256, 15 segundos de DSD estéreo correspondem a aproximadamente 42,3 MB de payload de áudio.

O Stability Mode não consegue compensar uma rede cuja velocidade sustentada seja inferior ao débito necessário pelo DSD. Nesse caso, deve ser usada Ethernet Gigabit e/ou reduzido o tráfego simultâneo.

### Armazenamento e cache

Quando é utilizada uma SACD ISO, o componente pode criar uma cache DSF antes/durante a preparação da transmissão. Recomenda-se um SSD para a cache, sobretudo quando existem outras actividades intensivas no disco.

Como referência aproximada de espaço para a cache local:

- DSD64: cerca de 20 MB/minuto
- DSD128: cerca de 40 MB/minuto
- DSD256: cerca de 81 MB/minuto

São valores aproximados para DSD estéreo; o sistema de ficheiros e o contentor DSF acrescentam algum espaço.

### T+A SDX 3100 HV

A documentação do SDX 3100 HV especifica LAN 10/100/1000 Base-T e indica DFF/DSF e DSD64, DSD128 e DSD256 para o Streaming Client. O DAC do aparelho suporta taxas DSD superiores através de outras entradas, mas isso não deve ser confundido com os formatos documentados para streaming pela rede.


## Compilação

O repositório não inclui a SDK do foobar2000. Utiliza a **SDK 2025-03-07** oficial.

Estrutura esperada:

```text
SDK-2025-03-07/
├── foobar2000/
│   ├── SDK/
│   ├── shared/
│   └── foo_sacd_dlna/
├── pfc/
└── libPPUI/
```

Abrir `foobar2000/foo_sacd_dlna/foo_sacd_dlna.sln` no Visual Studio 2022 e compilar `Release | x64`.

**Nota:** o código desta Alpha ainda não foi compilado neste ambiente, porque não está disponível aqui o ambiente MSVC/Visual Studio.

## Estado do DLNA

Quando o servidor está activo, a interface mostra:

```text
SACD DLNA:  BROADCASTING / ACTIVE
foo_input_sacd:  INSTALLED  <versão>
Music Library:  SHARING  (<N> DSD tracks)
Server: foobar2000 SACD DSD
HTTP port: 8192
```

`BROADCASTING / ACTIVE` significa que os serviços HTTP e SSDP estão activos. Não significa, por si só, que o T+A aceitou ou esteja actualmente a reproduzir uma faixa.

## T+A SDX 3100 HV

A T+A indica actualmente suporte de `DFF` e `DSF` e streaming DSD64/DSD128/DSD256 no Streaming Client do SDX 3100 HV. As entradas USB têm suporte para taxas superiores separadamente. ([T+A SDX 3100 HV](https://www.ta-hifi.de/en/audiosystems/hv-series/sdx-3100-reference-streaming-pre-dac/))

## Rede

- HTTP: `TCP 8192`
- SSDP: `239.255.255.250:1900`

Pode ser necessário criar uma regra no Windows Firewall para o foobar2000 na rede privada.

## Segurança

Esta Alpha foi concebida para uma rede local de confiança. O servidor HTTP não tem autenticação e não deve ser exposto directamente à Internet.

## Roadmap

- `BrowseMetadata` mais completo.
- Ajuste de `protocolInfo` para o firmware específico do SDX 3100 HV.
- Gapless mais robusto.
- DIDL-Lite mais completo.
- Melhor gestão de capas.
- Melhor concorrência e cancelamento.
- Gestão persistente do cache DSF.
- Actualização automática da Music Library partilhada.
- Diagnóstico e logging de rede mais detalhados.
- Builds automáticas Windows.
- Testes com o firmware exacto do T+A.

## Monitorização em tempo real

A V0.7 separa explicitamente o **broadcasting/descoberta DLNA** da **transmissão de áudio**:

- **DLNA discovery: BROADCASTING / ACTIVE** — os anúncios SSDP do servidor estão activos.
- **Audio stream: ACTIVE / TRANSMITTING** — um renderer DLNA está efectivamente a receber áudio por HTTP.
- **TX speed** — velocidade real medida dos dados enviados pela ligação TCP.
- **DSD rate** — taxa nominal DSD, quando conhecida.
- **DLNA client** — IP e identidade conhecida do cliente que está a receber o áudio.
- **T+A SDX** — é procurado através de SSDP/UPnP e aparece como `DETECTED / STREAMING` quando o IP detectado corresponde ao cliente que está a receber o áudio.

O áudio não é transmitido como um broadcast UDP. O SSDP serve para descoberta/anúncio; os dados de música são enviados ao renderer por HTTP.

## Stability Mode (V0.7)

A Stability Mode foi acrescentada para separar a conversão SACD ISO → DSD do caminho de rede. A ISO é convertida para DSF numa cache persistente e a transmissão só começa quando o ficheiro DSD está pronto. Antes de transmitir, o componente pode fazer um read-ahead configurável (5–60 s) e aumentar o buffer de envio TCP.

Isto ajuda a absorver picos curtos de carga do disco ou variações temporárias da rede. Não é possível garantir reprodução contínua se a largura de banda sustentada da rede ficar abaixo do débito necessário do DSD.

Débito estéreo aproximado: DSD64 = 5,64 Mbit/s; DSD128 = 11,29 Mbit/s; DSD256 = 22,58 Mbit/s.

## Current Alpha roadmap

The current alpha focuses on real renderer interoperability and diagnostics:

- complete `Browse` / `BrowseMetadata` and pagination
- renderer-specific DSD `protocolInfo` negotiation
- deterministic track order and duration metadata for gapless testing
- richer DIDL-Lite metadata and album art
- concurrent HTTP clients with cancellation
- persistent, invalidation-aware SACD→DSF cache
- Media Library callbacks and `GetSystemUpdateID`
- verbose Console + `network.log` diagnostics
- Windows GitHub Actions build packaging
- explicit T+A SDX 3100 HV firmware validation checklist

Gapless playback is deliberately marked as **renderer/firmware dependent** until it is tested on the exact SDX firmware.

## Documentação

- `HELP.md` — ajuda detalhada e resolução de problemas.
- `EXAMPLES.md` — exemplos práticos de reprodução e diagnóstico.
- `NETWORK_REQUIREMENTS.md` — requisitos de hardware e rede.
- `PREFERENCES_FIELDS.md` — referência rápida dos campos.
- `HARDWARE_VALIDATION.md` — matriz de validação do T+A SDX 3100 HV por firmware.
- `DLNA_TRACE_EXAMPLE.md` — sequência esperada de pedidos UPnP/DLNA.

## Real DLNA / renderer validation

The MediaServer path now separates SSDP discovery from real HTTP media transfer and records live renderer/TX status. Renderer capabilities are queried through UPnP `ConnectionManager::GetProtocolInfo` where available. Final T+A compatibility remains firmware-specific and must be validated on the exact SDX 3100 HV unit. See `HARDWARE_VALIDATION.md` and `tools/ta_sdx_probe.py`.

The Preferences page also provides **Clear DSF Cache** for invalidating generated DSF/manifests/artwork without touching source music.

See `PROTOCOL_COMPATIBILITY.md` for renderer negotiation details and `HARDWARE_VALIDATION.md` for exact-firmware testing.

## Referências

- [foobar2000 SDK](https://www.foobar2000.org/SDK)
- [T+A SDX 3100 HV](https://www.ta-hifi.de/en/audiosystems/hv-series/sdx-3100-reference-streaming-pre-dac/)
- [Super Audio CD Decoder](https://sourceforge.net/projects/sacddecoder/files/foo_input_sacd/)
