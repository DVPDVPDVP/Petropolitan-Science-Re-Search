import pandas as pd  # подключение библиотеки для работы

sample_file = 'sample.xlsx'  # входной и выходной файлы
result_file = 'result.xlsx'

emails = []
shifts_email = []
shift = 0
sample = pd.read_excel(sample_file)  # считываем входные данные
for i in range(sample.shape[0]):  # итерируемся по столбцу с почтами
    encoded_email = sample.iloc[i][0]  # получаем зашифрованную строку
    encoded_letter_1 = str(encoded_email).split('.')[-1][0]  # получаем первую букву доменного суффикса
    encoded_letter_2 = str(encoded_email).split('.')[-1][1]  # получаем вторую букву доменного суффикса
    shift_1 = (ord(encoded_letter_1) - ord('c')) % 26  # ищем сдвиг по первой букве
    if shift_1 < 0:
        shift_1 = 26 - shift_1
    shift_2 = (ord(encoded_letter_2) - ord('o')) % 26  # ищем сдвиг по первой букве
    if shift_2 < 0:
        shift_2 = 26 - shift_2
    if shift_2 == shift_1:  # если сдвиги равны - доменный суффикс .com, иначе - .org
        shift = shift_1
    else:
        shift = (ord(encoded_letter_1) - ord('o')) % 26
        if shift < 0:
            shift = 26 - shift

    res = ''
    for letter in encoded_email:  # теперь записываем расшифрованные по цезарю буквы в результат
        if letter.isalpha():
            if ord(letter) - shift < ord('a'):
                t = ord(letter) - ord('a')
                res += chr(ord('z') - (shift - t - 1))
            else:
                res += chr(ord(letter) - shift)
        else:
            res += letter
    emails.append(res)
    shifts_email.append(shift)

addresses = []
shifts_address = []
for i in range(sample.shape[0]):  # с адресами - тот же алгоритм
    encoded_address = str(sample.iloc[i][1])
    encoded_letter_1 = encoded_address.split(' ')[-1].split('.')[0][0]
    encoded_letter_2 = encoded_address.split(' ')[-1].split('.')[0][1]
    shift_1 = (ord(encoded_letter_1) - ord('к')) % 64
    if shift_1 < 0:
        shift_1 = 64 - shift_1
    shift_2 = (ord(encoded_letter_2) - ord('в')) % 64
    if shift_2 < 0:
        shift_2 = 64 - shift_2
    if shift_1 == shift_2:
        shift = shift_1
    else:  # если сдвиг по маленьким буквам не совпал - вычисляем по большим буквам
        shift = (ord(encoded_letter_1) - ord('К')) % 64
        if shift < 0:
            shift = 64 - shift
    res = ''
    for letter in encoded_address:
        if letter.isalpha():
            if ord(letter) - shift < ord('А'):
                t = ord(letter) - ord('А')
                res += chr(ord('я') - (shift - t - 1))
            else:
                res += chr(ord(letter) - shift)
        else:
            res += letter
    addresses.append(res)
    shifts_address.append(shift)

result = pd.DataFrame({  # формируем результирующий файл
    'Почта': emails,
    'Адрес': addresses,
    'Ключ': shifts_address
})

with pd.ExcelWriter(result_file, engine='openpyxl') as writer:
    result.to_excel(writer, index=False)
