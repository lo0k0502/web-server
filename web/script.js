const fileInput = document.getElementById('file');
const submitBtn = document.getElementById('submit');

submitBtn.onclick = async () => {
    const file = fileInput.files[0];
    if (!file) {
        console.error('No file');
        return;
    }

    const response = await fetch(
        'http://localhost:8080',
        {
            method: 'POST',
            body: file,
        },
    );
    console.log(response);
};