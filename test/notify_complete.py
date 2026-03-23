# -*- coding: utf-8 -*-
"""
编译完成语音提示脚本
"""
import win32com.client
import sys

def speak(text="任务运行完毕，过来看看！"):
    """使用Windows TTS播放语音"""
    try:
        speaker = win32com.client.Dispatch("SAPI.SpVoice")
        speaker.Speak(text)
    except Exception as e:
        print(f"语音播放失败: {e}")
        sys.exit(1)

if __name__ == "__main__":
    message = " ".join(sys.argv[1:]) if len(sys.argv) > 1 else "任务运行完毕，过来看看！"
    speak(message)
