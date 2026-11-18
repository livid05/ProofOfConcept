"""Inspect the actual linked ARM image, rather than a separate descriptor copy."""
from pathlib import Path
import struct, hashlib, json
root=Path(__file__).resolve().parents[1]
elf=(root/'build/keypad.elf').read_bytes()
assert elf[:7]==b'\x7fELF\x01\x01\x01','Expected ELF32 little endian'
assert struct.unpack_from('<H',elf,18)[0]==40,'Expected ARM machine'
shoff=struct.unpack_from('<I',elf,32)[0]
shsize,shnum=struct.unpack_from('<HH',elf,46)
sections=[struct.unpack_from('<10I',elf,shoff+i*shsize) for i in range(shnum)]
symbols={}
for sec in sections:
 if sec[1]!=2:continue
 strings=sections[sec[6]];names=elf[strings[4]:strings[4]+strings[5]]
 for off in range(sec[4],sec[4]+sec[5],sec[9]):
  name,value,size,info,other,index=struct.unpack_from('<IIIBBH',elf,off)
  label=names[name:].split(b'\0',1)[0].decode()
  if size and index<len(sections):
   target=sections[index];start=target[4]+value-target[3]
   symbols[label]=(value,elf[start:start+size])
cfg=symbols['cfg'][1];report=symbols['report_descriptor'][1];device=symbols['device'][1]
assert len(cfg)==34 and struct.unpack_from('<H',cfg,2)[0]==len(cfg)
offset=0;types=[]
while offset<len(cfg):
 length=cfg[offset];assert length>=2
 types.append(cfg[offset+1]);offset+=length
assert offset==len(cfg) and types==[2,4,0x21,5]
assert cfg[14:17]==bytes([3,1,1]),'HID boot keyboard class/subclass/protocol'
assert struct.unpack_from('<H',cfg,25)[0]==len(report),'HID report length'
assert cfg[33]==1, 'Initial endpoint interval must be the 8K HS / 1K FS value 1'
assert cfg[29]==0x81 and cfg[30]==3 and cfg[31:33]==bytes([8,0])
# Interpret HID short items to verify total input/output report bits.
size=count=bits_in=bits_out=collections=0;i=0
while i<len(report):
 prefix=report[i];i+=1
 assert prefix!=0xfe,'No HID long items expected'
 length=[0,1,2,4][prefix&3];value=int.from_bytes(report[i:i+length],'little');i+=length
 kind=(prefix>>2)&3;tag=prefix>>4
 if kind==1 and tag==7:size=value
 if kind==1 and tag==9:count=value
 if kind==1 and tag==8:raise AssertionError('Boot keyboard must have no report ID')
 if kind==0 and tag==8:bits_in+=size*count
 if kind==0 and tag==9:bits_out+=size*count
 if kind==0 and tag==10:collections+=1
 if kind==0 and tag==12:collections-=1
assert bits_in==64 and bits_out==8 and collections==0
assert device[0]==18 and device[7]==64 and device[17]==1
binary=(root/'build/keypad.bin').read_bytes()
stack,reset=struct.unpack_from('<II',binary)
assert stack==0x20270000 and reset&1 and 0x08000000<=reset<0x08000000+len(binary)
vectors=struct.unpack_from('<%dI'%(min(len(binary),1024)//4),binary)
for name in ['Reset_Handler','SysTick_Handler','OTG_HS_IRQHandler','HardFault_Handler']:
 address=symbols[name][0]|1
 assert address in vectors,name+' must be in the actual flash vector table'
for file in ['.vscode/tasks.json','.vscode/settings.json','.vscode/launch.json','Keypad-U5A5.code-workspace']:
 json.loads((root/file).read_text())
print('PASS: ARM ELF, flash vectors/stack, SysTick and USB IRQ vector, USB configuration chain, boot-keyboard interface, 8-byte input/1-byte output report, endpoint size, workspace/task JSON.')
print('BIN bytes:',len(binary),'SHA256:',hashlib.sha256(binary).hexdigest())
print('NOT TESTED: electrical operation, enumeration, physical switch typing.')
