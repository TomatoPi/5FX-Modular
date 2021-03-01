/*
 * Copyright (C) 2021 DAGO Kokri Esaïe <dago.esaie@protonmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

 ///
 ///  \file SynthFX.cpp
 /// 
 ///  \author DAGO Kokri Esaïe <dago.esaie@protonmail.com>
 ///  \date 2021-03-01
 ///

#include <5FX/jackwrap.hpp>

#include <jack/jack.h>
#include <jack/midiport.h>

#include <unordered_set>
#include <cstring>
#include <thread>
#include <chrono>
#include <cmath>

sfx::jack::Client client;
jack_port_t* midi_in;
jack_port_t* midi_out;

struct Note
{
  uint8_t channel;
  uint8_t note;
  uint8_t velocity;
};

bool operator== (const Note& a, const Note& b)
{
  return a.channel == b.channel && a.note == b.note;
}

template <>
struct std::hash<Note>
{
  constexpr size_t operator() (const Note& n) const
  {
    return n.channel | (n.note << 8);
  }
};

std::unordered_set<Note> notes;
uint64_t master_clock, note_start;

int tick_duration, note_length;

void add_note(uint8_t channel, uint8_t note, uint8_t velocity)
{
  Note n{ channel, note, velocity };
  notes.emplace(n);
  if (1 == notes.size()) {
    master_clock = note_start = 0;
  }
}

void remove_note(uint8_t channel, uint8_t note)
{
  Note n{ channel, note, 0 };
  notes.erase(n);
}

void clock_callback(jack_nframes_t frame, void* outbuffer)
{
  if (0 == master_clock % tick_duration) {
    note_start = master_clock;
    for (const auto& note : notes) {
      jack_midi_data_t* datas = jack_midi_event_reserve(outbuffer, frame, 3);
      datas[0] = 0x90 | note.channel;
      datas[1] = note.note;
      datas[2] = note.velocity;
    }
  }
  if (note_length <= master_clock - note_start) {
    for (const auto& note : notes) {
      jack_midi_data_t* datas = jack_midi_event_reserve(outbuffer, frame, 3);
      datas[0] = 0x80 | note.channel;
      datas[1] = note.note;
      datas[2] = 0;
    }
  }
  master_clock++;
}

static int process_callback(jack_nframes_t nframes, void* args)
{
  void* in_buffer = jack_port_get_buffer(midi_in, nframes);
  void* out_buffer = jack_port_get_buffer(midi_out, nframes);
  jack_midi_clear_buffer(out_buffer);

  jack_nframes_t nevents, i;
  jack_midi_event_t event;

  nevents = jack_midi_get_event_count(in_buffer);
  for (i = 0; i < nevents; ++i) {

    if (jack_midi_event_get(&event, in_buffer, i)) {
      continue;
    }
    if (3 != event.size) {
      continue;
    }

    int channel = event.buffer[0] & 0x0F;
    bool passthrough = true;

    switch (event.buffer[0] & 0xF0) {
    case 0x90:
      add_note(channel, event.buffer[1], event.buffer[2]);
      passthrough = false;
      break;
    case 0x80:
      remove_note(channel, event.buffer[1]);
      passthrough = false;
      break;
    case 0xF8:
      clock_callback(event.time, out_buffer);
      break;
    }

    if (passthrough) {
      jack_midi_data_t* datas = jack_midi_event_reserve(
        out_buffer, event.time, event.size);
      memcpy(datas, event.buffer, event.size * sizeof(jack_midi_data_t));
    }
  }
  return 0;
}

int main(int argc, char* const argv[])
{

  client.open(
    "ArpFX",
    {
    {&midi_in, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput},
    {&midi_out, "midi_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput},
    },
    process_callback);

  client.activate();

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 0;
}