void handleBotState(int64_t senderId, String text) {
  switch (botState) {
    case STATE_WAIT_AUTH_PASSWORD:
      if (text.toInt() == TB_pasword) {
        int i = user_find(0);
        if (i != 200) {
          users[i] = senderId;
          Save_Config(); 
          myBot.sendMessage(fb::Message(F("✅ Доступ надано!"), senderId));
          sendWelcomeMessage(senderId);
          sendMainMenu(senderId);
        } else {
          myBot.sendMessage(fb::Message(F("⚠️ Немає місця для нових користувачів."), senderId));
        }
      } else {
        myBot.sendMessage(fb::Message(F("❌ Невірний пароль."), senderId));
      }
      botState = STATE_IDLE;
      break;

    case STATE_WAIT_ALARM_CONFIRM:
      botState = STATE_IDLE;
      break;
      
    default:
      botState = STATE_IDLE;
      break;
  }
}
