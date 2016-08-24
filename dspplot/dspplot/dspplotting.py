from __future__ import division

import wave
import matplotlib
import matplotlib.ticker as tck
import matplotlib.pyplot as plt
import numpy as np
import sys
import matplotlib.colors as clr
from scipy import signal
from scipy import interpolate

def gen_ticks(stop, start=10):
    yield start
    for s in range(0, 10):
        if start * s > stop:
            yield stop
            raise StopIteration
        yield start * s
    for t in gen_ticks(stop, start * 10):
        yield t

def gen_tick_labels(stop, start=10):
    yield (str(start) + 'Hz').replace('000Hz', 'kHz')
    for s in range(0, 10):
        if start * s > stop:
            yield (str(int(stop)) + 'Hz').replace('000Hz', 'kHz')
            raise StopIteration
        yield ''
    for t in gen_tick_labels(stop, start * 10):
        yield t
        
def smooth_colormap(colors, name='cmap1'):
    to_rgb = clr.ColorConverter().to_rgb
    colors = [(p, to_rgb(c)) for p, c in colors]
    result = {'red': [], 'green': [], 'blue': []}
    for index, item in enumerate(colors):
        pos, color = item
        if pos is not None:
            r, g, b = color
            result['red'].append([pos, r, r])
            result['green'].append([pos, g, g])
            result['blue'].append([pos, b, b])
    cmap = clr.LinearSegmentedColormap(name, result)
    plt.register_cmap(name=name, cmap=cmap)
    return cmap

def wavplot(wavfile, title='Title', file=None, segmentsize=512, overlap=8):
    cmap = smooth_colormap([
        (0	, '#000000'),
        (1/9, '#010325'),
        (2/9, '#130246'),
        (3/9, '#51026e'),
        (4/9, '#9e0379'),
        (5/9, '#d6033e'),
        (6/9, '#fc4d21'),
        (7/9, '#fdc967'),
        (8/9, '#f3fab8'),
        (1  , '#ffffff')
        ])
        
    w = wave.open(wavfile, 'rb')

    sr = w.getframerate()
    data = np.fromstring(w.readframes(w.getnframes()), dtype=np.int32)/2147483647.0
    datalen = len(data)

    def fast_resample(data, newlen):    
        oldlen=len(data)
        result=[]
        for i in range(newlen):
            result.append(data[i*oldlen//newlen])
        return np.array(result)


    datalen = len(data)
    segments=datalen//segmentsize-1

    im=[]

    window = signal.hann(segmentsize * overlap)

    np.seterr(all='ignore')

    for segm in range(segments-overlap):
        r = range(segm*datalen//segments, segm*datalen//segments+segmentsize*overlap)
        subdata = data[r]
        subdata = subdata * window
        n = len(subdata)    
        Y = np.fft.fft(subdata)/n
        Y = Y[range(len(Y) // 2)]
        Yfreq = 20 * np.log10(np.absolute(Y))
        Yfreq = signal.resample(Yfreq, 512)
        Yfreq = np.fmax(-300, Yfreq)
        im.append(Yfreq)
        
    im = np.transpose(im)

    plt.imshow(im,cmap=cmap, aspect='auto', vmin=-160, vmax=0, origin='lower', extent=[0, datalen / sr, 0, sr / 2 ], interpolation='bicubic')
    plt.colorbar()
    
    if not file:
        plt.show()
    else:
        plt.savefig(file)
    

def plot(data,
         title='Title',
         horizontal=True,
         normalized_freq=False,
         Fs=48000,
         padwidth=1024,
         log_freq=False,
         file=None,
         freqresp=True,
         phaseresp=False,
         dots=False,
         segmentsize=512,
         overlap=8,
         div_by_N=False):
    if isinstance(data, (list, tuple, np.ndarray)):
        n = len(data)
        num = 1 + freqresp + phaseresp
        figsize = (10 if horizontal else 6 * num, 5 * num if horizontal else 6)
        fig, a = plt.subplots(num, 1, figsize=figsize) if horizontal else plt.subplots(1, num, figsize=figsize)
        fig.suptitle(title, fontsize=16)
        fig.subplots_adjust(top=0.85)
        rect = fig.patch
        rect.set_facecolor('#f0f0f0')
        style = {'linewidth': 1.4, 'color': '#0072bd'}
        grid_style = {'color': '#777777'}

        dataplot = a[0] if freqresp or phaseresp else a

        dataplot.plot(np.linspace(0, n, n, False), data, marker='.' if dots else None, **style)
        dataplot.set_xlabel('Samples')
        dataplot.set_ylabel('Amplitude')
        dataplot.grid(True, **grid_style)
        dataplot.set_autoscalex_on(False)
        dataplot.set_xlim([0, n - 1])
        dataplot.set_ylim(bottom=np.min(data))
        
        np.seterr(all='ignore')

        if freqresp or phaseresp:
            padwidth = max(padwidth, n)
            Y = np.fft.fft(np.pad(data, (0, padwidth - n), 'constant', constant_values=(0, 0)))
            Y = Y[range(padwidth // 2)]
            if div_by_N:
                Y = Y / n
            Yfreq = 20 * np.log10(np.abs(Y))
            Yfreq = np.fmax(-300, Yfreq)

            freq_label = [r'Normalized Frequency ($\times \pi$ rad/sample)', 'Frequency (Hz)']

            def set_freq(a):
                if normalized_freq:
                    a.set_xlabel(freq_label[0])
                    X = np.linspace(0, 1, len(Y), False)
                    a.set_xlim([0, 1])
                else:
                    a.set_xlabel(freq_label[1])
                    if log_freq:
                        a.set_xscale('log')
                        a.set_xticks(list(gen_ticks(Fs / 2)))
                        a.set_xticklabels(list(gen_tick_labels(Fs / 2)))
                    X = np.linspace(0, Fs / 2, len(Y), False)
                    a.set_xlim([10, Fs / 2])
                return X

            if freqresp:
                freqplot = a[1]
                X = set_freq(freqplot)
                freqplot.set_ylabel('Gain (dB)')
                freqplot.grid(True, **grid_style)
                freqplot.set_autoscalex_on(False)
                freqplot.plot(X, Yfreq, **style)

            if phaseresp:
                phaseplot = a[1 + freqresp]
                Yphase = np.angle(Y, deg=True);
                X = set_freq(phaseplot)
                phaseplot.grid(True, **grid_style)
                phaseplot.set_ylabel(r'Phase (${\circ}$)')
                phaseplot.set_autoscaley_on(False)
                phaseplot.set_ylim([-180, +180])
                phaseplot.plot(X, Yphase, **style)

        plt.tight_layout(rect=[0, 0.0, 1, 0.94])

        if not file:
            plt.show()
        else:
            plt.savefig(file)
    else:
        wavplot(data, title=title, file=file, segmentsize=segmentsize, overlap=overlap)

        
def perfplot(data, labels, title='Speed', xlabel='X', units='ms', file=None):
    
    styles = [
        {'color': '#F6511D', 'linestyle': '-', 'marker': 'o', 'markersize': 10.0, 'markeredgecolor': '#FFFFFF'},
        {'color': '#00A6ED', 'linestyle': '-', 'marker': 'o', 'markersize': 10.0, 'markeredgecolor': '#FFFFFF'},
        {'color': '#FFB400', 'linestyle': '-', 'marker': 'o', 'markersize': 10.0, 'markeredgecolor': '#FFFFFF'},
        {'color': '#7FB800', 'linestyle': '-', 'marker': 'o', 'markersize': 10.0, 'markeredgecolor': '#FFFFFF'},
        {'color': '#0D2C54', 'linestyle': '-', 'marker': 'o', 'markersize': 10.0, 'markeredgecolor': '#FFFFFF'},        
        ]
    grid_style = {'color': '#777777'}
    fig, ax = plt.subplots()
    ax.grid(True, **grid_style)
    data = map(list, zip(*data))
    ticks = data[0]
    data = data[1:]
    for d, s, l in zip(data, styles, labels):
        ax.set_xlabel(xlabel)
        ax.set_ylabel(units)
        x = np.linspace(0,len(d),len(d), False)
        ax.plot(x, d, linewidth=1.6, label=l, **s)   
        
    ax.set_ylim(bottom=0.0)
    legend = ax.legend(loc='lower center', shadow=True)
                
    plt.xticks(x, ticks, rotation='vertical')
    plt.tight_layout(rect=[0, 0.0, 1, 0.94])
        
    if not file:
        plt.show()
    else:
        plt.savefig(file)
