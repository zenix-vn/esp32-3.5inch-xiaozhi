#pragma once
// Quote of the day for the home screen: Vietnamese proverbs and sayings with an English rendering.
struct Quote {
  const char *vi, *en;
};
static const Quote QUOTES[] = {
    {"Có công mài sắt, có ngày nên kim.", "Keep grinding the iron bar and one day it becomes a needle."},
    {"Đi một ngày đàng, học một sàng khôn.", "Travel a day, gain a basketful of wisdom."},
    {"Uống nước nhớ nguồn.", "When you drink water, remember its source."},
    {"Một cây làm chẳng nên non, ba cây chụm lại nên hòn núi cao.", "One tree makes no hill; three trees together make a high mountain."},
    {"Thất bại là mẹ thành công.", "Failure is the mother of success."},
    {"Ăn quả nhớ kẻ trồng cây.", "When eating fruit, remember who planted the tree."},
    {"Kiến tha lâu đầy tổ.", "Ant by ant, the nest gets filled."},
    {"Lời nói chẳng mất tiền mua, lựa lời mà nói cho vừa lòng nhau.", "Words cost nothing, so choose kind ones."},
    {"Học thầy không tày học bạn.", "Learning from friends is worth as much as learning from teachers."},
    {"Gần mực thì đen, gần đèn thì rạng.", "Near ink you turn dark, near a lamp you shine."},
    {"Không thầy đố mày làm nên.", "Without a teacher, you could hardly succeed."},
    {"Nước chảy đá mòn.", "Running water wears away the stone."},
    {"Tốt gỗ hơn tốt nước sơn.", "Good wood matters more than good paint."},
    {"Chớ thấy sóng cả mà ngã tay chèo.", "Do not drop the oar because the waves are high."},
    {"Ai ơi giữ chí cho bền, dù ai xoay hướng đổi nền mặc ai.", "Keep your resolve firm, whatever others may do."},
    {"Muốn sang thì bắc cầu kiều, muốn con hay chữ thì yêu lấy thầy.", "To cross, build a bridge; to learn, honour your teacher."},
    {"Một con ngựa đau, cả tàu bỏ cỏ.", "When one horse is sick, the whole stable stops eating."},
    {"Lá lành đùm lá rách.", "The whole leaf wraps the torn one."},
    {"Bầu ơi thương lấy bí cùng, tuy rằng khác giống nhưng chung một giàn.", "Gourd, love the pumpkin: different kinds, same trellis."},
    {"Cẩn tắc vô áy náy.", "Being careful leaves no regrets."},
    {"Tham thì thâm.", "Greed brings loss."},
    {"Ăn trông nồi, ngồi trông hướng.", "Mind the pot when eating, mind the place when sitting."},
    {"Đói cho sạch, rách cho thơm.", "Even hungry, stay clean; even ragged, stay fragrant."},
    {"Có chí thì nên.", "Where there is a will, there is a way."},
    {"Chậm mà chắc.", "Slow but sure."},
    {"Một điều nhịn, chín điều lành.", "One moment of patience brings nine of peace."},
    {"Cái khó ló cái khôn.", "Difficulty brings out cleverness."},
    {"Thương người như thể thương thân.", "Love others as you love yourself."},
    {"Hôm nay là món quà, vì thế mới gọi là hiện tại.", "Today is a gift - that is why it is called the present."},
    {"Mỗi ngày một việc tốt, cuộc đời sẽ đẹp hơn.", "One good deed a day makes life more beautiful."},
    {"Nụ cười bằng mười thang thuốc bổ.", "A smile is worth ten tonics."},
    {"Muốn đi nhanh hãy đi một mình, muốn đi xa hãy đi cùng nhau.", "To go fast, go alone; to go far, go together."},
    {"Hành trình vạn dặm bắt đầu từ một bước chân.", "A journey of a thousand miles begins with a single step."},
    {"Đừng đếm những gì đã mất, hãy quý những gì đang có.", "Do not count what you lost; cherish what you have."},
    {"Sống là cho đâu chỉ nhận riêng mình.", "To live is to give, not only to receive."},
    {"Ngày mai bắt đầu từ hôm nay.", "Tomorrow starts today."},
    {"Biết đủ là đủ.", "Knowing what is enough is enough."},
    {"Người khôn nói ít, hiểu nhiều.", "The wise speak little and understand much."},
    {"Kiên nhẫn là chìa khóa của thành công.", "Patience is the key to success."},
    {"Hãy làm việc nhỏ với tình yêu lớn.", "Do small things with great love."},
};
static const int QUOTE_COUNT = sizeof(QUOTES) / sizeof(QUOTES[0]);

static const char *quoteOfDay(int yday) {
  const Quote &q = QUOTES[(yday * 7 + 3) % QUOTE_COUNT];  // stride so consecutive days differ in topic
  return isVi() ? q.vi : q.en;
}
