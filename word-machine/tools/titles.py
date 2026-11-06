"""Rule-based Vietnamese for book and chapter names. Longest phrase first; every result must fit the original byte length."""
import re
GRADE = {'一':'1','二':'2','三':'3','四':'4','五':'5','六':'6','七':'7','八':'8','九':'9'}
P = [
 ('人教精通版','PEP+ '),('苏教版','SJ '),('广东版','GD '),('陕西版','SX '),('湘鲁版','XL '),('天津新蕾版','TJ '),('新世纪版','NC '),('全册','Cả '),('综合教程','tổng hợp'),('视听说教程','Nghe nói'),('基础教程','Cơ bản'),('拓展阅读','Đọc thêm'),('全程辅导精读','Đọc sâu'),('图解速记','Nhớ qua hình'),('必考词','Bắt buộc'),('核心','Cốt lõi '),('牛津','Oxford '),('人教','PEP '),('新世纪','NC '),('新标准','Chuẩn mới '),('新目标','Mục tiêu mới '),('新编','Mới '),('全新版','Mới '),('进阶','Nâng cao '),('词组与','Cụm '),('十一','11'),('十','10'),
 ('各课词汇表 Lesson','Bài'),('词汇速记','Nhớ nhanh'),('牛津上海版','OxSH '),('北京版','BJ '),('鲁科版','LK '),('广州版','GZ '),('闽教版','MJ '),('川教版','SC '),('陕旅版','SL '),('辽师大版','LN '),('重大版','CQ '),('接力版','JL '),('清华版','TH '),('粤人版','YR '),('科普版','KP '),('仁爱版','RA '),('五四学制','5-4 '),('六三学制','6-3 '),
 ('小学英语大纲词汇','Từ vựng Tiểu học'),('初中英语大纲词汇','Từ vựng THCS'),('高中英语大纲词汇','Từ vựng THPT'),
 ('教材与考试说明差距词汇','Từ ngoài giáo trình'),('需要掌握的高频词汇','Từ tần suất cao'),('高中英语必备短语','Cụm từ THPT'),
 ('高分攻略关键词汇','Từ khóa điểm cao'),('初中已学基础词汇','Từ cơ bản THCS'),('新课标外重要词汇','Từ quan trọng'),('新课标新增词汇','Từ mới thêm'),
 ('核心词汇速记','Nhớ từ cốt lõi'),('基础常见简单词','Từ đơn giản'),('高考常考短语','Cụm từ thi'),('高中高频短语','Cụm từ thường gặp'),
 ('各课词汇表','Từ vựng bài'),('词组和短语','Cụm từ'),('阅读练习','Đọc'),('核心词汇','Từ cốt lõi'),('基础词汇','Từ cơ bản'),('高频词汇','Từ tần suất cao'),
 ('认知词汇','Từ nhận biết'),('常考词汇','Từ hay thi'),('生僻词汇','Từ hiếm'),('超纲词汇','Từ nâng cao'),('高分词汇','Từ điểm cao'),('必考词汇','Từ bắt buộc'),
 ('专有名词','Danh từ riêng'),('五级词汇表','Bảng cấp 5'),('二级词汇表','Bảng cấp 2'),('超纲词','Từ nâng cao'),('中频词','Tần suất vừa'),('高频词','Tần suất cao'),('低频词','Tần suất thấp'),
 ('基础词','Từ cơ bản'),('生僻词','Từ hiếm'),('简单词','Từ dễ'),('新课标','Chuẩn mới'),
 ('选择性必修','Tự chọn '),('必修','Bắt buộc '),('选修','Tự chọn'),('模块','Mô-đun'),
 ('全新版大学进阶英语综合教程','Tiếng Anh ĐH nâng cao'),('全新版大学英语综合教程','Tiếng Anh ĐH tổng hợp'),('新世纪英语专业综合教程','Chuyên Anh tổng hợp'),
 ('新视野大学英语读写教程','Đọc viết Anh ĐH'),('大学英语精读','Đọc sâu Anh ĐH'),
 ('新概念英语青少版','New Concept thiếu niên'),('新概念英语','New Concept'),('牛津高中英语','Oxford THPT'),
 ('人教版高中英语','PEP THPT'),('外研版高中英语','FLTRP THPT'),('北师大版高中英语','BNU THPT'),
 ('人教版','PEP '),('外研版','FLTRP '),('译林版','Yilin '),('教科版','EEC '),('北师大版','BNU '),('冀教版','Hebei '),('沪教版','SH '),('湘少版','Hunan '),('鲁教版','Lu '),('牛津版','Oxford '),
 ('第二版','bản 2'),('第三版','bản 3'),('第一部分','Phần 1'),('第二部分','Phần 2'),('第三部分','Phần 3'),('第四部分','Phần 4'),
 ('第一册',' tập 1'),('第二册',' tập 2'),('第三册',' tập 3'),('第四册',' tập 4'),
 ('全书','Cả sách'),('附录','Phụ lục'),('课文','Bài'),('对话','Hội thoại'),('短语','Cụm từ'),('词汇','Từ vựng'),
 ('小学','Tiểu học '),('初中','THCS '),('高中','THPT '),('大学','Đại học '),('英语','Anh '),
]
P += [('剑桥小学英语','Cambridge TH '),('剑桥国际少儿英语','Cambridge Kids '),('剑桥','Cambridge '),('JOIN版','JOIN '),('上外版','SISU '),('华研','Huayan '),('念念不忘乱序版','xáo trộn'),
      ('乱序版','xáo trộn'),('上海版','SH '),('上海','SH '),('北京','BJ '),('拓展阅读','Đọc thêm'),('全一册','Cả '),('初中英语','THCS Anh '),('高中英语','THPT Anh '),('高频词汇','Hay gặp'),
      ('常考词汇','Hay thi'),('居家生活','Đời sống'),('基础词','Cơ bản'),('基础词汇','Cơ bản'),('必修第','Bắt buộc tập '),('专业八级','TEM-8'),('专业八','TEM-8'),('专业四级','TEM-4'),('硕研','Cao học'),('考研','Thi cao học'),
      ('概念','Concept'),('旧','cũ'),('制','chế'),('课程要求','Yêu cầu'),('视野','Thị giới'),('视界','Thị giới'),('世纪','TK'),('标准','Chuẩn'),('目标','Mục tiêu'),('体验','Trải nghiệm'),('现代','Hiện đại'),('前景','Tiền cảnh'),('综合','tổng hợp'),('第','')]
P += [('星座与圣斗士','Cung HĐ'),('白羊座','Bạch Dương'),('金牛座','Kim Ngưu'),('双子座','Song Tử'),('巨蟹座','Cự Giải'),('狮子座','Sư Tử'),('处女座','Xử Nữ'),('天秤座','Thiên Bình'),('天蝎座','Bọ Cạp'),('射手座','Nhân Mã'),('摩羯座','Ma Kết'),('水瓶座','Bảo Bình'),('双鱼座','Song Ngư'),('圣衣传说','Thánh y'),
 ('地名','Địa danh'),('月份','Tháng'),('星期','Thứ'),('数词','Số từ'),('日常生活','Đời sống'),('与人相关','Về người'),('自然科学','Khoa học'),('社会政经','Xã hội'),('文体教育','Văn thể'),('物体描述','Tả vật'),('其他基础副词','Trạng từ'),('其他高频短语','Khác'),('其他','Khác'),
 ('学校与教育','Trường học'),('文化体育','Văn hóa'),('交通出行','Giao thông'),('商业娱乐','Thương mại'),('衣着食物','Ăn mặc'),('自然生命','Thiên nhiên'),('日常用语','Thường ngày'),('语法功能词','Ngữ pháp'),('介词短语','Giới từ'),('名词短语','Danh từ'),
 ('巧记活用','Nhớ khéo'),('巧记','Nhớ khéo '),('随身记','Mang theo'),('图解速记','Nhớ hình'),('同步词汇','Từ đồng bộ'),('高考英语词汇手册','Từ thi ĐH'),('高考词汇分频速记宝典便携版','Từ thi ĐH nhớ nhanh'),('高考词汇分频速记宝典','Từ thi ĐH nhớ nhanh'),
 ('高考','Thi ĐH '),('手册','sổ tay'),('便携手册','sổ tay'),('便携版','bỏ túi'),('考纲全攻略','Đề cương'),('名校名师','Danh sư'),('一笑而过','Cười trừ'),('白金版','Bạch kim'),('及时雨','Mưa kịp'),('思思大王','Si Si'),
 ('历年真题核心高频','Đề thi cốt lõi'),('核心高频','Cốt lõi'),('核心扩展','Mở rộng'),('洞穿词汇','Thấu từ'),('零频词','Tần suất 0'),('参考词汇表','Từ tham khảo'),('词组表','Cụm từ'),('课程教学要求','Yêu cầu'),('教学要求','Yêu cầu'),('学生用书','SHS'),('修订版','sửa đổi'),('基础篇','cơ bản'),('基础上','cơ bản 1'),('基础下','cơ bản 2'),('基础版','cơ bản'),('视听说','Nghe nói'),('高级英语','Anh cao cấp'),('简单词汇','Từ dễ'),('差距词汇','Từ chênh'),('课标外常考词汇','Ngoài chuẩn'),('课标外','Ngoài chuẩn'),('乱序','xáo'),('正序版','xuôi'),
 ('便携','bỏ túi'),('白金','Bạch kim'),('词汇','Từ vựng')]
P.sort(key=lambda x: -len(x[0]))
def tr(t):
    t=re.sub(r'([一二三四五六七八九])年级([上下])册',lambda m:f'L{GRADE[m[1]]} kỳ {1 if m[2]=="上" else 2}',t)
    t=re.sub(r'高([一二三])([上下])',lambda m:f'L{10+'一二三'.index(m[1])+1-1+0} kỳ {1 if m[2]=="上" else 2}'.replace('L10','L10'),t) if False else t
    t=re.sub(r'高([一二三])([上下])',lambda m:f'L{10+'一二三'.index(m[1])} kỳ {1 if m[2]=="上" else 2}',t)
    t=re.sub(r'(\d+)页',r'',t)
    t=re.sub(r'([一二三四五六七八九])年级起点',lambda m:f'từ L{GRADE[m[1]]} ',t)
    t=re.sub(r'([一二三四五六七八九])年级([上下])',lambda m:f'L{GRADE[m[1]]} kỳ {1 if m[2]=="上" else 2}',t)
    t=re.sub(r'([一二三四五六七八九])年级',lambda m:f'Lớp {GRADE[m[1]]}',t)
    t=re.sub(r'第(\d+)周',r'Tuần \1',t); t=re.sub(r'第(\d+)版',r'bản \1',t); t=re.sub(r'(\d+)页',r'tr.\1',t)
    t=re.sub(r'(\d+)课',r'\1 bài',t); t=re.sub(r'(\d+)词',r'\1 từ',t)
    for a,b in P: t=t.replace(a,b)
    t=t.replace('（','(').replace('）',')').replace('上册','tập 1').replace('下册','tập 2')
    for a,b in (('模块','Mô-đun'),('Mô-đun一','Mô-đun 1'),('Mô-đun二','Mô-đun 2'),('Mô-đun三','Mô-đun 3'),('Mô-đun四','Mô-đun 4'),('Mô-đun五','Mô-đun 5'),('Mô-đun六','Mô-đun 6'),('Mô-đun七','Mô-đun 7'),('Mô-đun八','Mô-đun 8'),('Mô-đun九','Mô-đun 9'),('册',''),('版','')):
        t=t.replace(a,b)
    t=re.sub(r'  +',' ',t).strip()
    return t

SHORT = [('Hay gặp','HG'),('Cambridge','Cb'),('Thi cao học','Cao học'),('Yêu cầu','YC'),('tổng hợp','TH'),('Trải nghiệm','TN'),('Hiện đại','HĐ'),('Cơ bản','CB'),('Bắt buộc tập','BB'),('xáo trộn','xáo'),('Lesson','Bài'),('Chapter','Ch'),('Chatper','Ch'),('Unit','Bài'),('List','DS'),('Part','P'),('PART','P'),('Cả sách','Cả'),('Từ điểm cao','Điểm cao'),('Từ nâng cao','Nâng cao'),('Từ bắt buộc','BB'),('Nhớ nhanh','Nhớ'),('thiếu niên','TN'),('New Concept','NCE'),('Mô-đun','MĐ'),('Từ vựng','TV'),('Tần suất','TS'),('Cụm từ','CT'),('Từ cốt lõi','Cốt lõi'),('Từ cơ bản','Cơ bản'),('Từ nhận biết','Nhận biết'),('Bắt buộc','BB'),('Tự chọn','TC'),('Danh từ riêng','Tên riêng'),('Tiểu học','TH'),('Đại học','ĐH'),('Hội thoại','HT'),('tần suất cao','TS cao'),('kỳ ','k'),(' từ L',' L')]


SKIP = re.compile('表示|名词$|形容词|动词|性质|状态|行为|场所|地点|情况')
_ASCII = re.compile(r"[A-Za-z0-9][A-Za-z0-9.\-/&'()]*")


def _phrases(orig, vi):
    """Split Vietnamese into ASCII tokens kept as on the original and phrases drawn as cells."""
    prot = set(_ASCII.findall(orig))
    out, buf = [], []
    for tok in vi.split(' '):
        if not tok:
            continue
        if tok in prot:
            if buf:
                out.append(('c', ' '.join(buf)))
                buf = []
            out.append(('a', tok))
        else:
            buf.append(tok)
    if buf:
        out.append(('c', ' '.join(buf)))
    return out


def size(orig, vi):
    """Bytes the title will take on the card."""
    parts = _phrases(orig, vi)
    n = sum(len(s) if k == 'a' else 3 * (-(-len(s) // 2)) for k, s in parts)
    return n + max(0, len(parts) - 1)


def encode(pool, orig, vi):
    return ' '.join(s if k == 'a' else pool.line(s) for k, s in _phrases(orig, vi))


def fit_cells(orig, limit):
    """Vietnamese for a title that fits `limit` bytes, shortening step by step; None if it cannot."""
    if SKIP.search(orig) and '专有' not in orig:
        return None
    vi = tr(orig)
    for a, b in [(None, None)] + SHORT:
        if a:
            vi = vi.replace(a, b)
        if not re.search('[一-鿿]', vi) and size(orig, vi) <= limit:
            return vi
    if re.search('[一-鿿]', vi):
        return None
    toks = vi.split(' ')
    while len(toks) > 1 and size(orig, ' '.join(toks)) > limit:
        toks.pop()
    return ' '.join(toks) if size(orig, ' '.join(toks)) <= limit else None
