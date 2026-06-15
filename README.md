# 🎚️ AudioCompressor VST3

> *"Comprimir áudio sem entender o que está acontecendo é como apertar parafuso com faca de manteiga."*
> — Prof. Thomaz Chaves de Andrade Oliveira, provavelmente

Plugin VST3 de compressão dinâmica desenvolvido como projeto prático da disciplina **Aplicações de Processamento Digital de Áudio Profissional** — UFLA, sob orientação do **Prof. Thomaz Chaves de Andrade Oliveira**. Implementação completa em C++/JUCE com detector linked-stereo, soft knee configurável e interface gráfica customizada.

---

## 🧠 O que é um compressor de áudio?

Um compressor é um processador de **dinâmica**: ele reduz automaticamente o ganho do sinal quando sua amplitude ultrapassa um limiar definido (threshold). O resultado prático é um sinal com menos variação entre os picos e os vales — sons mais altos são atenuados, enquanto sons mais baixos permanecem intactos.

### Os conceitos físicos por trás

**Amplitude e pressão sonora**
O sinal de áudio digital representa variações de pressão do ar ao longo do tempo. Cada amostra (*sample*) é um valor entre −1.0 e +1.0, proporcional à pressão instantânea. Quando esse valor ultrapassa o threshold, o compressor intervém.

**dBFS (decibéis relativos à escala cheia)**
O nível do sinal é medido em dBFS: 0 dBFS é o pico máximo possível sem clipping; valores negativos indicam atenuação. A relação é logarítmica — um sinal a −6 dBFS tem metade da amplitude linear de um sinal a 0 dBFS.

$$L_{dBFS} = 20 \cdot \log_{10}(|A|)$$

**Ratio (razão de compressão)**
Define o quanto o sinal acima do threshold é atenuado. Com ratio 4:1, para cada 4 dB que o sinal excede o threshold, apenas 1 dB passa. Com ∞:1, temos um limitador — nada ultrapassa o threshold.

$$GR_{dB} = (\text{inputDb} - T) \cdot \left(1 - \frac{1}{R}\right)$$

**Soft Knee**
A transição entre "sem compressão" e "com compressão" pode ser abrupta (hard knee) ou suavizada (soft knee). Com soft knee de largura *W*, a compressão começa a agir gradualmente na região $[T - W/2,\ T + W/2]$, usando uma curva quadrática:

$$GR_{knee} = \frac{(1/R - 1) \cdot (x)^2}{2W}, \quad x = \text{inputDb} - T + \frac{W}{2}$$

Isso elimina distorção de modulação perceptível em transientes.

**Ataque e Release (constantes de tempo)**
O envelope follower suaviza a resposta do compressor no tempo. As constantes são calculadas como filtros de primeira ordem sobre a frequência de amostragem:

$$\alpha_{attack} = e^{-1 / (0.001 \cdot t_{ms} \cdot f_s)}$$

- **Attack**: tempo para o compressor reagir a um transiente de entrada (1–200 ms)
- **Release**: tempo para o compressor "soltar" após o sinal cair abaixo do threshold (10–2000 ms)

Attack rápido + release lento → controle agressivo, pode "sugar" transientes.
Attack lento + release rápido → preserva punch, soa mais natural.

**Linked Stereo**
O detector usa o *pico máximo entre todos os canais* a cada sample. Isso garante que L e R recebam exatamente o mesmo ganho em cada instante — sem imagem estéreo sendo deslocada pela compressão assimétrica.

**Makeup Gain**
Como a compressão reduz o nível médio, o makeup gain compensa essa perda, restaurando o loudness percebido. Aplicado em escala linear após o envelope:

$$G_{total} = G_{envelope} \cdot 10^{G_{makeup}/20}$$

---

## 🛠️ Stack tecnológico

| Tecnologia | Papel |
|---|---|
| **C++17** | Linguagem base — desempenho determinístico no audio thread |
| **JUCE 8.0.13** | Framework de plugins: DSP, GUI, APVTS, formato VST3 |
| **VST3 SDK** (via JUCE) | Formato de plugin — compatível com DAWs modernas |
| **AudioProcessorValueTreeState** | Estado de parâmetros thread-safe com serialização XML |
| **Projucer** | Geração de projeto Visual Studio 2022 |
| **Visual Studio 2022** | Compilador MSVC, target Windows x64 |

---

## 🎨 Interface

A UI foi desenhada do zero com `LookAndFeel_V4` customizado — sem componentes padrão do JUCE visíveis.

**Background**
Gradiente deep purple-black (`#120C20` → `#1C1430`) com grid de linhas sutis em 4% de opacidade. Acento lateral roxo sólido no header. Separador horizontal com glow.

**Knobs**
Cada `LabeledKnob` é um `Component` composto por:
- Anel de trilha escuro como base
- Anel de valor com gradiente roxo (`#643CC8` → `#B464FF`) que cresce junto com o parâmetro
- Corpo central com gradiente radial simulando profundidade
- Ponteiro branco com ponto brilhante no centro
- Label do parâmetro acima, valor numérico formatado abaixo

**GR Meter**
Medidor vertical de Gain Reduction com gradiente verde → amarelo → vermelho de cima para baixo. Implementa peak hold simples: sobe instantaneamente, desce com fator de suavização 0.93 por frame a 30 Hz. Exibe o valor em dB abaixo da barra.

```
┌──────────────────────────────────────────────────────────────┐
│ ▌ COMPRESSOR                                            v2.0 │
│   Dynamics Processor  |  Linked Stereo  |  Soft Knee        │
│ ─────────────────────────────────────────────────────────── │
│  THRESHOLD  RATIO   ATTACK  RELEASE  KNEE   MAKEUP  │  GR  │
│    (◎)      (◎)     (◎)     (◎)      (◎)    (◎)    │ ████ │
│   -18dB     4:1    10ms    150ms    6dB     0dB    │      │
└──────────────────────────────────────────────────────────────┘
```

---

## ⚙️ Parâmetros

| Parâmetro | Range | Default | Descrição |
|---|---|---|---|
| Threshold | −60 → 0 dB | −18 dB | Nível a partir do qual a compressão começa |
| Ratio | 1:1 → 20:1 | 4:1 | Razão de compressão (skew 0.5 para controle fino nos valores baixos) |
| Attack | 1 → 200 ms | 10 ms | Tempo de resposta ao transiente de entrada |
| Release | 10 → 2000 ms | 150 ms | Tempo de recuperação após o sinal cair |
| Knee | 0 → 12 dB | 6 dB | Largura da zona de transição suave |
| Makeup | 0 → 24 dB | 0 dB | Ganho de compensação pós-compressão |

---

## 🏗️ Arquitetura do código

```
AudioCompressor/
├── Source/
│   ├── PluginProcessor.h / .cpp   # DSP: compressor, envelope, APVTS
│   └── PluginEditor.h / .cpp      # GUI: LabeledKnob, GRMeter, LookAndFeel
├── AudioCompressor.jucer           # Configuração do Projucer
└── Builds/VisualStudio2022/        # Gerado pelo Projucer (não versionado)
```

**Thread model**
- `processBlock` roda no audio thread: sem alocações, sem locks. Parâmetros lidos via `getRawParameterValue()->load()` (atômico). GR exposto via `std::atomic<float>`.
- `timerCallback` no editor roda na message thread a 30 Hz: lê o atômico e dispara repaint.

---

## 🚀 Build

**Pré-requisitos**
- Visual Studio 2022 (com Desktop Development with C++)
- JUCE 8.0.13 extraído em `~/Downloads/juce-8.0.13-windows/`
- Projucer (incluído no JUCE)

**Passos**
```bash
# 1. Abrir AudioCompressor.jucer no Projucer e salvar (gera a solução)
# 2. Abrir Builds/VisualStudio2022/AudioCompressor.sln
# 3. Build → Release
# O .vst3 gerado pode ser copiado para C:\Program Files\Common Files\VST3\
```

---

## 📚 Referências

- Zölzer, U. — *DAFX: Digital Audio Effects*, Wiley, 2011
- Giannoulis, D. et al. — *Digital Dynamic Range Compressor Design*, JAES, 2012
- JUCE Framework Documentation — https://juce.com/learn/documentation
- Disciplina **Aplicações de Processamento Digital de Áudio Profissional** — UFLA
  Prof. **Thomaz Chaves de Andrade Oliveira**
