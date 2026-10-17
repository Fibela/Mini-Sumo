# Interruptor, botón y conectores

Lo que falta del montaje eléctrico. No está en la cotización de PIGRA y sin esto el robot
no se puede armar ni presentar a verificación técnica.

---

## 1. Interruptor principal

### Por qué importa más de lo que parece

En MiniSumo **está prohibido el paro remoto**: no hay botón de emergencia inalámbrico. El
interruptor físico es la única forma de apagar el robot si algo sale mal, así que el juez
lo va a buscar en la verificación del Día 1.

Y hay una trampa: por él pasa **toda** la corriente del robot, incluidos los picos de
arranque de los motores. Un interruptor de señal se funde o se queda pegado.

### Qué comprar

| Requisito | Valor | Por qué |
|---|---|---|
| Tipo | SPST (un polo, un tiro) | Solo corta el positivo |
| Corriente | **Mínimo 3 A** | Los dos motores piden picos de ~1.5 A; el margen es obligatorio |
| Formato | **Palanca con tuerca de rosca** | Se atornilla al chasis y no se mueve con los golpes |
| Diámetro de rosca | 6 mm (tipo MTS-101) | Entra en una perforación estándar |

**El que yo compraría:** interruptor de palanca miniatura **MTS-101**, SPST, 3 A / 250 V,
rosca de 6 mm. Cuesta alrededor de **$25–40 MXN** y se consigue en cualquier tienda de
electrónica.

### Lo que NO sirve

- **Switches deslizantes pequeños** (los de plástico, tipo SS-12D00): se mueven solos con la
  vibración de los impactos. Un robot que se apaga a mitad del combate pierde por detención.
- **Switches de panel de 2 A**: quedan justos y se calientan.
- **Botones momentáneos usados como interruptor**: no mantienen el estado.

### Cómo y dónde montarlo

Va **en serie con el positivo de la batería**, antes de cualquier otra cosa: antes del
INA219, antes del buck y antes del driver.

```
Batería (+) ──→ [ INTERRUPTOR ] ──→ resto del robot
Batería (−) ──→ masa común
```

Se atornilla a la **pared lateral o trasera del chasis**, con la palanca accesible sin
desarmar nada. Conviene orientarlo de modo que la palanca quede **paralela** a la dirección
de los golpes: así un impacto frontal no lo apaga.

Cable de **AWG 20 o 22 de silicón** a ambos lados. El silicón importa porque soporta el
calor del cautín sin derretirse y es mucho más flexible que el PVC.

---

## 2. Botón de arranque

### Qué tiene que hacer

El reglamento exige que el robot espere **exactamente 5 segundos** después de pulsar el
botón antes de moverse. Ese botón es lo último que tocas antes de retirarte del dohyo.

### Qué comprar

| Requisito | Valor | Por qué |
|---|---|---|
| Tipo | Momentáneo, **normalmente abierto** (NO) | Solo cierra mientras lo presionas |
| Formato | **Pulsador de panel de 12 mm con tuerca** | Se atornilla; el táctil de 6 mm se despega |
| Corriente | Cualquiera, 50 mA basta | Solo lleva señal lógica |

**El que yo compraría:** pulsador de panel momentáneo de **12 mm**, normalmente abierto,
con tuerca de montaje. Cuesta **$20–35 MXN**. Si consigues uno con aro luminoso, mejor
todavía: te indica que el robot está encendido.

### Alternativa que no cuesta nada

**Con ESP32:** el botón **BOOT** de la placa ya es un pulsador momentáneo conectado a
GPIO 0. Funciona perfectamente como botón de arranque y no hay que comprar nada. La única
precaución: **no presionarlo al encender**, porque entonces el ESP32 entra en modo de
programación en lugar de arrancar el firmware.

**Con Pico:** el botón BOOTSEL no sirve para esto, porque está cableado a la memoria flash.
Aquí sí hace falta un pulsador externo.

### Cómo conectarlo

Dos cables, sin resistencias externas:

```
GPIO del microcontrolador ──→ [ BOTÓN ] ──→ GND
```

El pull-up interno del microcontrolador se encarga del resto. En el código:

```cpp
pinMode(PIN_BOTON, INPUT_PULLUP);
// El pin lee ALTO en reposo y BAJO cuando se presiona.
```

**Rebote.** Cuando presionas un botón, el contacto vibra durante unos milisegundos y el
microcontrolador puede leer varias pulsaciones. Se resuelve en firmware ignorando las
lecturas de los primeros 20 ms. No hace falta ningún componente.

### Dónde montarlo

En la **cara superior o trasera** del chasis, donde puedas alcanzarlo con el robot ya
colocado en su posición de salida. Si queda debajo o entre los sensores, vas a tener que
mover el robot para encenderlo — y moverlo después de colocarlo no siempre está permitido.

---

## 3. Conectores: el fin de los jumpers

### El problema

Los cables dupont (los jumpers de colores con punta macho o hembra) **no tienen retención
mecánica**: se sostienen solo por fricción. En combate el robot recibe golpes a un metro por
segundo, y un cable de I²C que se sale a mitad del segundo round es un combate perdido sin
que sepas por qué.

Son perfectos para la protoboard. En el robot armado, no.

### La solución: JST-XH

El **JST-XH** es un conector con clip de retención: hace clic al insertarse y hay que
presionar la pestaña para sacarlo. Es el estándar en robótica de competencia.

| Tamaño | Para qué |
|---|---|
| 2 vías | Alimentación de 5 V del buck, botón |
| 3 vías | Cada sensor de línea (VCC, GND, OUT) |
| 4 vías | Bus I²C (3V3, GND, SDA, SCL) |
| 8 vías | Señales al driver |

### Qué comprar

| Artículo | Cantidad | Precio aprox. |
|---|---|---|
| Kit de conectores JST-XH surtido (2, 3, 4, 5 vías) con carcasas y terminales | 1 kit | $80–120 |
| **Pinza crimpadora para JST** (SN-01BM o similar) | 1 | $250–400 |
| Cable de silicón AWG 26, varios colores | 10 m | $60 |
| Cable de silicón AWG 22, rojo y negro | 3 m | $40 |

**Sobre la crimpadora:** es la compra que más duele y la que más se agradece. Sin ella, los
terminales crimpados con pinzas normales se salen al tirar del cable — que es exactamente lo
que no queremos. Si el presupuesto no da, hay dos salidas honestas:

1. **Soldar directo** las uniones permanentes (sensores de línea, cables de motor) y usar JST
   solo donde de verdad necesites desconectar.
2. Buscar **latiguillos JST-XH ya crimpados de fábrica** — vienen con el cable puesto y solo
   hay que empalmarlos. Cuestan más por unidad pero evitan comprar la herramienta.

### Código de colores

Mantener el mismo criterio en todo el robot ahorra horas de depuración:

| Color | Señal |
|---|---|
| Rojo | Positivo de batería (7.4 V) |
| Naranja | 5 V del buck |
| Rosa o blanco | 3.3 V |
| **Negro** | **GND, siempre. Nunca otra cosa** |
| Amarillo | SDA |
| Azul | SCL |
| Verde | Señales digitales (XSHUT, dirección de motores) |
| Morado | Señales analógicas (sensores de línea) |

### Reglas de cableado

- **Cables de motor trenzados** entre sí. Dos cables trenzados cancelan buena parte del campo
  magnético que generan y dejan de meter ruido en los sensores.
- **Separar potencia de señal.** Los cables de motor y batería por un lado del chasis, los de
  sensores por el otro. Si tienen que cruzarse, que sea en ángulo recto, nunca en paralelo.
- **Amarrar los mazos al chasis** con cincho o con las propias perforaciones. Nada suelto: un
  cable que se mueve es un cable que termina en una rueda.
- **Dejar un poco de holgura** en cada conector, pero no un bucle grande. Lo suficiente para
  poder desconectar sin forzar.

---

## 4. Lista de compra completa

Todo lo que falta para terminar el montaje eléctrico:

| # | Artículo | Cant. | Precio aprox. | Prioridad |
|---|---|---|---|---|
| 1 | **Convertidor buck 5 V** (MP1584 o LM2596) | 1 | $30–50 | **Crítica** |
| 2 | Interruptor de palanca MTS-101, SPST 3 A, rosca 6 mm | 1 | $25–40 | **Crítica** |
| 3 | Pulsador de panel 12 mm, momentáneo NO | 1 | $20–35 | Alta *(gratis con ESP32: usar BOOT)* |
| 4 | Kit de conectores JST-XH surtido | 1 | $80–120 | Alta |
| 5 | Cable de silicón AWG 26, surtido de colores | 10 m | $60 | Alta |
| 6 | Cable de silicón AWG 22, rojo y negro | 3 m | $40 | Alta |
| 7 | Perfboard de tira perforada 7 × 5 cm | 2 | $50 | Alta |
| 8 | Tiras de header macho y hembra 2.54 mm | 1 juego | $40 | Alta |
| 9 | Condensador electrolítico 470 µF 16 V | 2 | $16 | Alta |
| 10 | Resistencias 4.7 kΩ | 10 | $5 | Alta |
| 11 | Resistencias 100 kΩ y 47 kΩ (divisor de batería) | 5 c/u | $5 | Media |
| 12 | Insertos térmicos M2 y M3 | 20 c/u | $90 | Media |
| 13 | Pinza crimpadora JST SN-01BM | 1 | $250–400 | Media |
| 14 | Bolsa ignífuga para batería | 1 | $150–280 | **Obligatoria por reglamento** |
| | **Total sin crimpadora** | | **$611–831** | |
| | **Total con crimpadora** | | **$861–1 231** | |

Todo se consigue en los mismos proveedores: UNIT Electronics, 330ohms, AG Electrónica o
la misma PIGRA donde compraste lo demás.

---

## 5. Lo que hay que añadir al chasis

Tres cosas que el modelo CAD todavía no tiene y que conviene agregar antes de imprimir:

| Qué | Dónde | Para qué |
|---|---|---|
| Perforación de 6 mm | Pared lateral o trasera | Interruptor de palanca |
| Perforación de 12 mm | Cara superior o trasera | Botón de arranque |
| 2 insertos M2 | Pared trasera | Sujetar el driver TB6612 de canto |

Son tres operaciones rápidas sobre el árbol de SolidWorks que ya existe, o dos líneas en la
macro `cad/macros/MiniSumo_Chasis_v2.bas`.
