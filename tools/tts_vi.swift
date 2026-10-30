// Speaks a text with a macOS voice chosen by identifier and writes it to an audio file (CAF, the engine's format).
// `say -v` cannot pick the downloadable neural voices; AVSpeechSynthesizer can. Used by tools/gen_letter_voice.py.
//
//   swift tools/tts_vi.swift <voice identifier> <rate 0..1> <jobs.tsv>   # one "out.caf<TAB>text" per line
//   swift tools/tts_vi.swift --list                                     # the Vietnamese voices installed
// (Run it with `swift`, not as a compiled binary: a compiled one did not see the downloaded voices.)
import AVFoundation
import Foundation

let args = CommandLine.arguments
if args.count > 1 && args[1] == "--list" {
  for v in AVSpeechSynthesisVoice.speechVoices() where v.language.hasPrefix("vi") {
    print("\(v.identifier)\t\(v.name)\tquality \(v.quality.rawValue)")
  }
  exit(0)
}
guard args.count >= 4, let voice = AVSpeechSynthesisVoice(identifier: args[1]), let rate = Float(args[2]),
  let jobs = try? String(contentsOfFile: args[3], encoding: .utf8)
else {
  print("usage: tts_vi.swift <voice id> <rate 0..1> <jobs.tsv>   (or --list)")
  exit(1)
}
let synth = AVSpeechSynthesizer()
var failed = 0
for line in jobs.split(separator: "\n") {
  let parts = line.split(separator: "\t", maxSplits: 1).map(String.init)
  guard parts.count == 2 else { continue }
  let u = AVSpeechUtterance(string: parts[1])
  u.voice = voice
  u.rate = rate
  var file: AVAudioFile?
  var done = false
  synth.write(u) { buffer in
    guard let pcm = buffer as? AVAudioPCMBuffer else { return }
    if pcm.frameLength == 0 {  // the end
      done = true
      return
    }
    if file == nil {
      file = try? AVAudioFile(forWriting: URL(fileURLWithPath: parts[0]), settings: pcm.format.settings,
                              commonFormat: pcm.format.commonFormat, interleaved: pcm.format.isInterleaved)
    }
    try? file?.write(from: pcm)
  }
  let deadline = Date().addingTimeInterval(30)
  while !done && Date() < deadline { RunLoop.main.run(until: Date().addingTimeInterval(0.02)) }
  file = nil
  if !done { failed += 1 }
}
exit(failed == 0 ? 0 : 2)
