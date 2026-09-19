# foo_sacd_dlna — Roadmap

Versão actual: **0.8-alpha1** (ver `VERSION`, `CHANGELOG.md`).

Este ficheiro é a única fonte de verdade sobre o estado do projecto e o trabalho
planeado. O `README.md` e o `README.pt-PT.md` remetem para aqui em vez de
manterem listas próprias.

Legenda: **FEITO** (presente na árvore) · **PARCIAL** (implementado, incompleto
ou não validado) · **PLANEADO** (não implementado) · **BLOQUEADO**.

---

## 0. Bloqueio actual — o componente não compila

`Debug/foo_sacd_dlna.log`:

```text
foobar2000\helpers\foobar2000-lite+atl.h(15,10): fatal error C1083:
não é possível abrir o ficheiro incluir: 'atlapp.h': No such file or directory
```

`atlapp.h` é um cabeçalho da **WTL**, não da ATL. Não vem com o Visual Studio e
não é instalado por nenhum componente "C++ ATL". Os helpers do SDK e a `libPPUI`
exigem a WTL no caminho de includes.

| # | Item | Estado |
|---|------|--------|
| 0.1 | Obter a WTL 10.x e colocá-la onde o projecto agora a espera (`../../WTL/Include`, já acrescentado a `AdditionalIncludeDirectories` nas duas configurações). A `libPPUI` precisa do mesmo caminho. | **BLOQUEADO — fazer primeiro** |
| 0.2 | Corrigir o diagnóstico no `BUILD.md` e no `V142_TOOLSET_CHANGE.md`, que atribuíam a falha à falta do componente ATL v142 | **FEITO (0.8-alpha2)** |
| 0.3 | Reavaliar a descida de toolset `v143 → v142`, aplicada com base numa causa mal identificada | **PLANEADO** |
| 0.4 | Rebuild limpo completo em `Debug|x64` e `Release|x64` e registo do primeiro build bem-sucedido no `CHANGELOG.md` | **PLANEADO** |

Nada do que se segue pode ser validado antes de 0.1 estar resolvido. Nenhum item
deste ficheiro foi alguma vez executado em hardware.

---

## 1. Entregue (0.5 → 0.8-alpha1)

Verificado no código-fonte:

| Item | Onde | Estado |
|------|------|--------|
| Descoberta SSDP, descrições de dispositivo/serviços, entrega HTTP | `dlna_server.cpp` | **FEITO** |
| `Browse` com `BrowseDirectChildren` e `BrowseMetadata` | `browseDidl`, `browseResponse` | **FEITO** |
| Paginação (`StartingIndex` / `RequestedCount` / `NumberReturned` / `TotalMatches`) | `browseDidl` | **FEITO** |
| `GetSystemUpdateID` e callbacks de alteração da Media Library | `library_tracker` | **FEITO** |
| Metadados DIDL-Lite com papéis de artista e `albumArtURI` | `appendTrack` | **FEITO** |
| Cache persistente de capas com invalidação pela fonte | `dlna_server.cpp` | **FEITO** |
| Cache persistente SACD→DSF com manifesto e invalidação por fonte/descodificador/versão | `dsf_writer.*`, ficheiros `.partial` | **FEITO** |
| Concorrência limitada (`kMaxConcurrentStreams = 2`) com cancelamento via `abort_callback` e limpeza dos `.partial` | `dlna_server.cpp` | **FEITO** |
| Suporte de `Range` HTTP | `serveMedia` | **FEITO** |
| Stability Mode: read-ahead e buffer de envio TCP aumentado | `config.*`, `preferences.cpp` | **FEITO** |
| Estado em tempo real: taxa TX, taxa DSD, IP do cliente, detecção T+A, tamanho da cache | `status.h`, `ui_element.cpp` | **FEITO** |
| Diagnóstico na Consola e em `network.log` com timestamps | `dlna_server.cpp` | **FEITO** |
| Integração opcional com `foo_dsd_processor`, com fingerprint do preset na invalidação da cache | `dsp_bridge.*` | **FEITO** |
| Acção `Clear DSF Cache` | `preferences.cpp` | **FEITO** |
| Ferramentas de diagnóstico | `tools/*.py` | **FEITO** |

Estes itens continuavam listados como *planeados* nos dois READMEs, embora o
`CHANGELOG.md` os desse como entregues em 0.7-alpha2/alpha3. É essa contradição
que justifica a existência deste ficheiro.

---

## 2. Implementado mas não validado

| Item | Nota | Estado |
|------|------|--------|
| Negociação de `protocolInfo` em função do renderer | O `ConnectionManager::GetProtocolInfo` é consultado e o Sink negociado é guardado, mas nunca foi analisada uma resposta de um renderer real | **PARCIAL** |
| Entrega DSD64 / DSD128 / DSD256 | nunca reproduzido em hardware | **PARCIAL** |
| Descodificação de ISO SACD via `foo_input_sacd` | nunca executada | **PARCIAL** |
| Caminho do DSD Processor (PCM→DSD, DSD256→DSD128) | nunca executado; a verificação de saída nativa DSD está por testar | **PARCIAL** |
| Detecção do T+A SDX 3100 HV | lógica existe, unidade nunca esteve presente | **PARCIAL** |

Cada linha corresponde a uma caixa por assinalar no `RELEASE_CHECKLIST.md`.

---

## 3. Planeado — completude do protocolo

| Item | Estado |
|------|--------|
| O handler do `ConnectionManager` responde a **qualquer** POST em `/ctl/ConnectionManager` com um `GetProtocolInfoResponse` fixo; discriminar por SOAPACTION e acrescentar `GetCurrentConnectionIDs` / `GetCurrentConnectionInfo` | **PLANEADO** |
| `GetSortCapabilities` e `GetSearchCapabilities` (pedidos por vários renderers no arranque) | **PLANEADO** |
| `X_GetFeatureList` (lista de funcionalidades DLNA) | **PLANEADO** |
| O `protocolInfo` de `Source` é estático; derivá-lo do que o servidor consegue efectivamente servir | **PLANEADO** |
| O contentor de raiz expõe apenas `Artists`; acrescentar `Albums`, `All Tracks` e, eventualmente, `Folders` | **PLANEADO** |
| `SortCriteria` é aceite e ignorado | **PLANEADO** |
| `Filter` é aceite e ignorado; respeitá-lo ou devolver sempre o conjunto completo de propriedades de forma deliberada | **PLANEADO** |
| Ordem determinística das faixas e duração fiável nos metadados (pré-requisito para testar gapless) | **PLANEADO** |
| Mais formatos e tamanhos de capa | **PLANEADO** |

---

## 4. Planeado — build, empacotamento e release

| Item | Estado |
|------|--------|
| A pasta `.github/workflows/` não existe na árvore, apesar de o `CHANGELOG.md` 0.7-alpha2 e o `README.md` descreverem um workflow GitHub Actions para Windows; ou se cria, ou se retira a afirmação | **PLANEADO** |
| Empacotamento automático do `foo_sacd_dlna.fb2k-component` | **PLANEADO** |
| Instruções de build reprodutíveis, incluindo a dependência da WTL | **PLANEADO** |
| Fixar o snapshot do SDK usado nos builds de release | **PLANEADO** |

---

## 5. Planeado — validação em hardware (dependente da secção 0)

Executada contra o `HARDWARE_VALIDATION.md` e o `RELEASE_CHECKLIST.md`:

| Item | Estado |
|------|--------|
| Registar a versão exacta do firmware do SDX 3100 HV em teste | **PLANEADO** |
| Capturar o `protocolInfo` real do Sink devolvido pelo renderer | **PLANEADO** |
| Reprodução de DSF DSD64 / DSD128 / DSD256 | **PLANEADO** |
| Reprodução de ISO SACD através do `foo_input_sacd` | **PLANEADO** |
| Transferência DSD256 sustentada durante mais de 30 minutos | **PLANEADO** |
| Comportamento gapless registado como PASS/FAIL — **depende do renderer/firmware, nunca se assume** | **PLANEADO** |
| Confirmar que não há conversão DSD→PCM em nenhum ponto do caminho de rede | **PLANEADO** |
| Verificar as instruções de Windows Firewall numa máquina limpa | **PLANEADO** |

---

## 6. Metas por versão

| Versão | Âmbito |
|--------|--------|
| 0.8-alpha2 | Apenas a secção 0: dependência da WTL documentada e a compilar, documentação de build corrigida, primeiro `Release|x64` limpo |
| 0.9-alpha1 | Completude de protocolo da secção 3; CI e empacotamento da secção 4 |
| 0.9-beta | Secção 5 executada no SDX 3100 HV real; `RELEASE_CHECKLIST.md` integralmente assinalado |
| 1.0 | Beta estável no firmware validado, com o gapless documentado como PASS ou FAIL e não como pendente |

---

## 7. Defeitos conhecidos na documentação

Não são trabalho de roadmap, mas devem ser corrigidos em conjunto:

- O read-ahead é indicado como **5–60 s** no `README.md` e no `README.pt-PT.md`
  e como **5–120 s** no `CHANGELOG.md`. Um dos dois está errado.
- O `README.pt-PT.md` rotula a Stability Mode e a monitorização em tempo real
  como **V0.7**, o `README.md` rotula as mesmas secções como **V0.5** / **V0.7**
  e o `CHANGELOG.md` introduz a Stability Mode em **0.6 Alpha 1**.
- Falta o `RELEASE_NOTES_0.6.md`; existem os de 0.5, 0.7 e 0.8.
- O `README.pt-PT.md` ainda tem a secção "Current Alpha roadmap" em inglês.
